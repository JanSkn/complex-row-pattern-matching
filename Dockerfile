# alpine instead of ubuntu to reduce image size

# ==== build image ====
FROM alpine:latest AS build

WORKDIR /app

COPY . .

# Alpine uses apk
# install make, libcurl, run make
# curl-dev instead of libcurl4-openssl-dev as Alpine instead of Ubuntu
# no ssl needed
RUN apk add --no-cache \
   g++ \
   make \
   curl-dev && \
   make clean && \
   make
# =====================

# === runtime image ===
FROM alpine:latest

WORKDIR /app

# only copy required files and directories
COPY --from=build /app/config.json .
COPY --from=build /app/dbrex/config ./dbrex/config
COPY --from=build /app/dbrex/data ./dbrex/data
COPY --from=build /app/main .
COPY --from=build /app/dbrex/python ./dbrex/python
COPY --from=build /app/start.sh .

# set env variable PROD_ENV to avoid make command in start.sh
ENV PROD_ENV=true

# install dependencies (netcat for nc in start.sh)
# netcat-openbsd for Alpine instead of netcat
# python venv required by newer Alpine versions to protect system Python
# bash must be installed
# activate venv with . as it is POSIX conform
RUN apk add --no-cache \
    libcurl \
    python3 \
    py3-pip \
    bash \
    netcat-openbsd \
    && python3 -m venv /venv \  
    && . /venv/bin/activate \
    && pip install -r dbrex/python/requirements.txt

# add venv to path so that all python commands (in start.sh) use the venv
ENV PATH="/venv/bin:$PATH"  

RUN chmod +x /app/start.sh

ENTRYPOINT ["/app/start.sh"]

# run with args
CMD ["$@"]
# =====================
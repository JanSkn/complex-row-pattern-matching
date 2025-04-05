import pandas as pd

df = pd.read_csv("benchmarks/dataset/Montreal_Crime_Data.csv", index_col=0)

df['date'] = pd.to_datetime(df['date'])
df = df.sort_values(by='date')

df = df.reset_index(drop=True)
df.insert(0, 'id', range(len(df)))

df.to_csv("benchmarks/dataset/Montreal_Crime_Data.csv", index=False)
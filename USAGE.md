# Database Connection Guide for DBrex

## Overview
DBrex connects to your database through Trino (formerly PrestoSQL), which acts as a distributed SQL query engine. This guide will help you set up and configure your database connection.

## Understanding Trino Architecture 🐰

### Data Source Organization
Trino uses a three-level hierarchy to organize data sources:
- **Catalog**: Configuration for accessing a specific data source
- **Schema**: Logical grouping of tables within a catalog
- **Table**: Individual data tables

This hierarchy is reflected in Trino's fully qualified naming: `catalog.schema.table`

### Catalogs Explained
A catalog in Trino represents a configured data source and consists of:
- Connector configuration
- Authentication credentials
- Connection properties
- Data source URL

Catalogs are defined through properties files in Trino's configuration directory. The filename becomes the catalog name (e.g., `postgres.properties` creates a catalog named "postgres").

## Setting Up a Connection to your Database

### 1. Start Trino Container
Launch a mounted Trino container:
```bash
docker run --name trino \
    -d \
    -p 8080:8080 \
    -v <YOUR_PATH>:/etc/trino/catalog \
    trinodb/trino
```

### 2. Configure Your Connector
1. Choose your connector from the [official Trino connectors list](https://trino.io/docs/current/connector.html)
2. Create a properties file in your mounted configuration directory:
   - Filename format: `<connector_name>.properties`
   - Example: `postgres.properties`, `mysql.properties`, ...
3. Add the required configuration parameters for your chosen connector

Example PostgreSQL connector configuration:
```properties
connector.name=postgresql
connection-url=jdbc:postgresql://example.net:5432/database
connection-user=root
connection-password=secret
```

### 3. Verify Connection
1. Restart the Trino container:
```bash
docker restart trino
```

2. Connect to Trino CLI and verify your catalog:
```bash
docker exec -it trino trino
```
```sql
SHOW catalogs;
```

3. Explore your data source:
```sql
-- List schemas in your catalog
SHOW SCHEMAS FROM catalog_name;

-- List tables in a schema
SHOW TABLES FROM catalog_name.schema_name;
```

#### Example

<img width="863" alt="image" src="https://github.com/user-attachments/assets/1e5a72a2-9413-4c2f-adf9-d2aeb37ed230" />

## DBrex Setup

To give DBrex access to your database, you must pass the catalog, the schema and the table name along with other parameters when starting the DBrex container.
There are two options to pass the arguments:

1. as command line arguments
```bash
docker run -d -v dbrex:/app/dbrex/data --name dbrex ghcr.io/janskn/dbrex:latest <args>
```

2. In a file called `args.json`.
```bash
docker run -d -v dbrex:/app/dbrex/data -v <YOUR_PATH>:/app/dbrex/config --name dbrex ghcr.io/janskn/dbrex:latest
```
The argument names must be equivalent to the long option below. You can find an example [here](example/example_args.json).

> [!WARNING]
> The columns in `-q` or `--queries` must be in format `symbol_name.column_name` as in the [example](example/example_args.json).

Here is a list of all arguments:

| Option | Long Option | Description |
|--------|-------------|-------------|
| -c | --catalog | Name of the data source |
| -s | --schema | Name of the schema of the catalog |
| -t | --table_name | Name of the source table |
| -r | --regex | RegEx pattern. Explicit use of parentheses, example: 'A1 A2 \| A3' evaluates to '(A1 A2) \| A3', instead use 'A1 (A2 \| A3)' |
| -a | --alphabet | RegEx alphabet |
| -q | --queries | SQL queries separated by RegEx symbol. Must be same order as the alphabet |
| -o | --output_table | Name of the output table |
| -oc | --output_columns | Columns of the output table |
| -twt | --time_window_type | Time window type (countbased or sliding_window). Only countbased supported at this version |
| -twc | --tw_column | Name of the column for the time window selection. Values must be integer and in increasing order |
| -tws | --tw_size | Time window size |
| -sf | --sleep_for | sleep for n milliseconds after each execution to conserve resources |

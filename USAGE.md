# Usage 

To connect your database to DBrex, you need to configure the SQL Engine.

## Trino 🐰

### Catalog and Schema

A Trino catalog is a collection of configuration properties used to access a specific data source, including the required connector and any other details such as credentials and URL. Catalogs are defined in properties files stored in the Trino configuration directory. The name of the properties file determines the name of the catalog. For example, the properties file `etc/example.properties` results in a catalog name `example`.

Schemas are a way to organize tables. Together, a catalog and schema define a set of tables and other objects that can be queried.

### Setup

See a list of all [supported connectors](https://trino.io/docs/current/connector.html).

Start a mounted Trino Container to configure the connectors:

```bash
$ docker run --name trino -d -p 8080:8080 -v <YOUR_PATH>:/etc/trino/catalog trinodb/trino
```

Afterwards, create a connector file for your data source and store it on the host where you mounted the container.

The name should be in the following format: `connector_name.properties`, e.g. `postgres.properties`.

The configurations depend on the data source, find the corresponding one [here](https://trino.io/docs/current/connector.html).

After you created the properties file, restart the Trino container.

If you set up everything correctly, you can see your data source when entering 
```bash 
SHOW catalogs;
```
in Trino.

### Example

The basic properties file for PostgreSQL looks like this:

```properties
connector.name=postgresql
connection-url=jdbc:postgresql://<YOUR_HOST>:5432/<YOUR_DATABASE>
connection-user=<YOUR_USER>
connection-password=<YOUR_PASSWORD>
```

### DBrex Setup

To give DBrex access to your database, you must pass the catalog, the schema and the table name when starting the DBrex container.

You can find a list of the corresponding arguments in the [ReadMe](README.md).

The exact structure depends on the data source.

To find out, enter

```bash
SHOW schemas FROM <YOUR_DATA_SOURCE>;
```
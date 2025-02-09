# DBrex 👋

Introducing **DBrex**, a light-weighted **pattern matching algorithm** for **databases** that uses regular expressions and deterministic finite automata (DFA) to efficiently detect patterns in SQL tables.

![Demo](./demo.gif)

## Key Features ⭐

- ⚙️ **Effortless Setup**: Install seamlessly using Docker.
- 🚀 **Performance Boost**: Storing intermediate states allows quick evaluations of new rows without recomputing. 
- 🎯 **Pattern Detection**: Utilize regular expressions and DFA-based transitions to find complex patterns.
- 🔄 **Incremental Processing**: Leverage existing results when new data arrives, eliminating full recomputation.
- 🤝 **SQL Integration**: Generate results through efficient JOIN operations based on DFA transitions.

## Quick Start with Docker 🐳

> [!NOTE]
> DBrex will listen to your database table as long as the container is running. Stop the container if not needed.

> [!WARNING]
> When using Docker, make sure to mount a volume with `-v dbrex:/app/src/data` in your Docker command. This step is crucial as without it DBrex restarts all already executed computations.

**Example Docker Command**:

```bash
docker run -d -v dbrex:/app/src/data -e --name dbrex ghcr.io/dbrex:latest ...(variables)
```

## How It Works 🛠️

DBrex employs a unique approach to pattern matching in databases:

1. Pattern Definition: Patterns are defined using regular expressions, making them intuitive and flexible
2. DFA Translation: Regular expressions are converted into deterministic finite automata
3. SQL Integration: DFA transitions are mapped to SQL JOIN operations
4. State Management: Intermediate states are preserved to optimize performance
5. Incremental Updates: New data is processed using existing results, avoiding full recomputation

### Supported Databases 🗄️
DBrex is built on top of the SQL engine [Trino](https://trino.io).

Included SQL dialects are:

- MySQL
- PostgreSQL
- MariaDB

And many more...

Check the [Trino documentation](https://trino.io/ecosystem/data-source.html) for a complete list of supported databases and data sources. 

## Comparison with other Technologies

### Complex Event Processing (CEP) Engines
While traditional CEP engines offer support for data streams, DBrex provides:

- Native SQL database integration
- Efficient state management for incremental processing
- No need for separate processing engine

### SQL MATCH_RECOGNIZE
Compared to SQL's MATCH_RECOGNIZE:

- Better performance through state preservation
- Simpler syntax and easier maintenance
- Efficient handling of new data through incremental processing

## Use Cases 🎯

- Data Monitoring: DBrex is ideal for scenarios where new data is continuously added, and patterns need to be detected in close-to-real-time without reprocessing the entire dataset.
- Log Analysis: Efficiently identify patterns in log data stored in SQL tables, such as detecting sequences of errors or specific user activities.
- Financial Transactions: Detect fraudulent patterns or anomalies in transaction data as new records are inserted.
- Trend analysis: Deriving future trends from past data.

## Limitations ⚠️

Regular expression operators `*` (Kleene star) and `+` (Plus) are currently not supported in pattern definitions.
Patterns are limited to finite-length sequences.

## License 📜

This project is licensed under the MIT License.

<br><br>

Created by [Jan Skowron](https://github.com/janskn)
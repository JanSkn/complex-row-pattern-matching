<div align="center">
  <a href="https://github.com/FrameworkV/FoodFusionAI">
    <img src="https://github.com/user-attachments/assets/ef557a6f-0644-4e7c-b8d4-a11da164d30b" alt="Logo" width="200" height="200">
  </a>
</div>

# DBrex 👋

![Version](https://img.shields.io/github/v/release/JanSkn/complex-row-pattern)
![GitHub top language](https://img.shields.io/github/languages/top/janskn/complex-row-pattern)

Introducing **DBrex**, a light-weighted **pattern matching application** for **databases** that uses regular expressions and deterministic finite automata (DFA) to efficiently detect patterns in SQL tables.

Once the container is running, the application finds all patterns in the table and returns them in an output table, optimised for frequently updated databases.

![Demo](demo.gif)

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
> 1) When using Docker, make sure to mount a volume with `-v dbrex:/app/dbrex/data` in your Docker command. This step is crucial as without it DBrex restarts all computations after container restart.
>
> 2) If you are on Linux, make sure to add `--add-host=host.docker.internal:host-gateway`

**Example Docker Command**:

```bash
docker run -d -v dbrex:/app/dbrex/data --name dbrex ghcr.io/janskn/dbrex:latest <args>
```

## Setup & Usage

To set up DBrex with your own database and to start the container, follow this [guide](USAGE.md#dbrex-setup).

## How It Works 🛠️

DBrex employs a unique approach to pattern matching in databases:

1. Pattern Definition: Patterns are defined using regular expressions, making them intuitive and flexible
2. DFA Translation: Regular expressions are converted into deterministic finite automata
3. SQL Integration: DFA transitions are mapped to SQL JOIN operations
4. State Management: Intermediate states are preserved to optimize performance
5. Incremental Updates: New data is processed using existing results, avoiding full recomputation

<img src="/animations/DFA_traversal.gif" width="300">

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

- Simpler syntax and easier maintenance
- Efficient handling of new data through incremental processing

## Use Cases 🎯

- Data Monitoring: DBrex is ideal for scenarios where new data is continuously added, and patterns need to be detected in close-to-real-time without reprocessing the entire dataset.
- Log Analysis: Efficiently identify patterns in log data stored in SQL tables, such as detecting sequences of errors or specific user activities.
- Financial Transactions: Detect fraudulent patterns or anomalies in transaction data as new records are inserted.
- Trend analysis: Deriving future trends from past data.

## Limitations ⚠️

Your table must contain a column with integers in ascending order with a step size of 1.

Regular expression operators `*` (Kleene star) and `+` (Plus) are currently not supported in pattern definitions due to JOIN constraints.

Patterns are limited to finite-length sequences.

## Hint 💡

You can see the magic happen at http://127.0.0.1:8080/ui/# - to log in, enter an arbitrary username.

## Benchmarks

Find performance evalutations [here](benchmarks/output).

Underlying data set is from [kaggle](https://www.kaggle.com/datasets/stevieknox/montreal-crime-data).

## License 📜

This project is licensed under the MIT License.

<br><br>

Created by [Jan Skowron](https://github.com/janskn)

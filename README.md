# Sparkland Assignment - Real-time Market Data Processor

A C++ application that subscribes to Coinbase WebSocket feeds, processes ticker data in real-time, calculates Exponential Moving Averages (EMA), and logs all data to CSV files.

### Key Components

- **CoinbaseClient**: WebSocket client
- **TickParser**: JSON parser using SimdJSON
- **EMA**: Exponential Moving Average calculator with configurable time periods
- **RingBuffer**: Lock-free circular buffer for single producer consumer
- **CSVLogger**: Asynchronous CSV file writer
- **Logger**: Thread-safe application logging

## EMA Calculation Method

This implementation uses a **time-decay approach** for calculating Exponential Moving Averages, if we need sliding window over 5 sec interval approach EMA class can be updated accordingly.

### Time-Decay EMA Formula

```
EMA(t) = α × Price(t) + (1 - α) × EMA(t-1)

where:
α (alpha) = 1 - e^(-Δt / time_period)
Δt = current_time - previous_update_time
time_period = 5.0 seconds
```

## Prerequisites

### System Requirements
- **Compiler**: GCC 9+ or Clang 7+ (C++17 support required)
- **CMake**: Version 3.14 or higher
- **Boost**: Version 1.86 or higher

## Quick Start

### 1. Build and Run

```bash
git clone <repository-url>
cd sparkland_assignment

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the application
make -j$(nproc)

# Run the main application
./sparkland_app
```

### 2. Check Output
```bash
# View generated CSV file
head -5 ticks.csv

# Check application logs
tail -f application.log
```

## Running Tests

### Quick Test Run
```bash
cd build
./sparkland_tests
```

## Configuration

### Subscribed Products
The application subscribes to these cryptocurrency pairs by default:
- **BTC-USD**
- **ETH-USD**
- **SOL-USD**

To modify, edit `src/main.cpp`:
```cpp
std::vector<std::string> products = {
    "BTC-USD",
    "ETH-USD", 
    "SOL-USD",
    // Add more pairs here
};
```

### Buffer Capacity
Ring buffer size can be adjusted in `include/sparkland/types.h`:
```cpp
constexpr size_t TICK_BUFFER_CAPACITY = 1024; // Adjust as needed
```

## Further Optimizations

- Parse only required fields to double and keep the rest in string to avoid unecessary copy, or `from_chars` can be used for conversion to avoid copy (mac-os doesn't support it as of now for double)
- Pin threads to specific cores
- Create a ConfigReader class to change configuration like product ids, csv path, buffer size etc without recompilation.



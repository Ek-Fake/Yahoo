# Use official PHP CLI image
FROM php:8.0-cli

# Install necessary dependencies: gcc (for compiling C code), curl, and procps (for pkill)
RUN apt-get update && apt-get install -y \
    build-essential \
    procps \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy PHP files and C source code into the container
COPY . /app

# Set the working directory to /app
WORKDIR /app

# Compile your C program (replace program.c with the actual name of your C source file)
RUN gcc program.c -o compiled_program

# Expose port 8080 for PHP built-in server
EXPOSE 8080

# Start the PHP built-in server on port 8080
CMD ["php", "-S", "0.0.0.0:8080", "-t", "/app"]

# Use official PHP image
FROM php:8.0-cli

# Install required dependencies for compiling C code
RUN apt-get update && apt-get install -y \
    build-essential \
    libpthread-stubs0-dev \
    gcc \
    make \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy PHP and C files into the Docker image
COPY . /app

# Set the working directory to /app
WORKDIR /app

# Compile the C program with pthread
RUN gcc -o compiled_program program.c -lpthread

# Expose port for PHP built-in server
EXPOSE 8080

# Command to run PHP server
CMD ["php", "-S", "0.0.0.0:8080", "-t", "/app"]

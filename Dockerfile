FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    git \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN g++ -std=c++17 main.cpp -o charu_bot -lcurl -lpthread

CMD ["./charu_bot"]


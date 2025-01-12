#ifndef SYNARTISIS_H
#define SYNARTISIS_H

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define NUM_PRODUCTS 20
#define MAX_ORDER 10
#define NUM_CUSTOMERS 5

typedef struct {
    char description[50];
    float price;
    int item_count;
} Product;

// Initialize product catalog with random data
void initialize_catalog();

// Process individual order
void process_order(int product_id, int pipe_write);

// Serve a single customer
void serve_customer(int pipe_read, int pipe_write);

// Customer process
void customer(int customer_id, int pipe_read, int pipe_write);

#endif

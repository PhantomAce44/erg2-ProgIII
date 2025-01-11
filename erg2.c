#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_PRODUCTS 20
#define MAX_ORDER 10
#define NUM_CUSTOMERS 5

typedef struct {
    char description[50];
    float price;
    int item_count;
} Product;

Product catalog[NUM_PRODUCTS];

int total_requests = 0;
int total_successful = 0;
int total_failed = 0;
int total_products_requested = 0;
int total_products_sold = 0;
float total_revenue = 0.0;

// Initialize product catalog with random data
void initialize_catalog() {
    const char* product_names[] = {"Widget", "Gadget", "Tool", "Device", "Item", "Accessory", "Appliance", "Thing"};
    srand(time(NULL));  // Seed the random number generator

    for (int i = 0; i < NUM_PRODUCTS; i++) {
        // Randomly generate product description
        int name_index = rand() % 8;
        int suffix = rand() % 100;  // Random number to make each description unique
        snprintf(catalog[i].description, sizeof(catalog[i].description), "%s %d", product_names[name_index], suffix);

        // Randomize price between 10 and 100
        catalog[i].price = 10.0 + (rand() % 91);  // Price between 10 and 100

        // Randomize item count between 1 and 5
        catalog[i].item_count = rand() % 5 + 1;
    }
}

// Process individual order
void process_order(int product_id, int pipe_write) {
    if (product_id >= NUM_PRODUCTS || catalog[product_id].item_count <= 0) {
        write(pipe_write, "Order Failed", sizeof("Order Failed"));
    } else {
        catalog[product_id].item_count--;
        char response[100];
        snprintf(response, sizeof(response), "Order Success, Total: %.2f", catalog[product_id].price);
        write(pipe_write, response, strlen(response) + 1);
    }
}

// Serve a single customer
void serve_customer(int pipe_read, int pipe_write) {
    int product_id;
    while (read(pipe_read, &product_id, sizeof(product_id)) > 0) {
        total_products_requested++;
        if (product_id >= NUM_PRODUCTS || catalog[product_id].item_count <= 0) {
            total_failed++;
        } else {
            total_successful++;
            total_products_sold++;
            total_revenue += catalog[product_id].price;
        }
        process_order(product_id, pipe_write);
    }
}

// Customer process
void customer(int customer_id, int pipe_read, int pipe_write) {
    srand(customer_id);
    float customer_total = 0.0;  // Accumulate total purchases for this customer

    for (int i = 0; i < MAX_ORDER; i++) {
        int product_id = rand() % NUM_PRODUCTS;
        write(pipe_write, &product_id, sizeof(product_id));

        char response[100];
        read(pipe_read, response, sizeof(response));

        // If order was successful, extract the price and add to customer_total
        if (strstr(response, "Order Success") != NULL) {
            float price;
            sscanf(response, "Order Success, Total: %f", &price);
            customer_total += price;
        }

        // Sleep for 1 second before placing the next order
        sleep(1);  // Adds a 1-second delay between each order
    }

    // Print the customer's total after all their orders
    printf("Client %d: Purchase complete, your total is: %.2f euro.\n", customer_id, customer_total);
}

// Main function
int main() {
    initialize_catalog();

    int request_pipes[NUM_CUSTOMERS][2];
    int response_pipes[NUM_CUSTOMERS][2];

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if (pipe(request_pipes[i]) == -1 || pipe(response_pipes[i]) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(request_pipes[i][0]);
            close(response_pipes[i][1]);
            customer(i, response_pipes[i][0], request_pipes[i][1]);
            exit(0);
        } else {
            close(request_pipes[i][1]);
            close(response_pipes[i][0]);
            serve_customer(request_pipes[i][0], response_pipes[i][1]);
        }
    }

    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        wait(NULL);
    }

    // Final output summary
    printf("\n%d requests were made, where %d succeeded and %d failed\n", 
           total_products_requested, total_successful, total_failed);
    printf("%d products were requested, where %d products were bought, totaling %.2f euros\n",
           total_products_requested, total_products_sold, total_revenue);

    return 0;
}

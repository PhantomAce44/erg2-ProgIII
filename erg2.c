#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_PRODUCTS 20
#define MAX_ORDER 10

// Δομή για τα προϊόντα
typedef struct {
    char description[50];
    float price;
    int item_count;
} Product;

// Κατάλογος προϊόντων
Product catalog[NUM_PRODUCTS];

// Αρχικοποίηση του καταλόγου προϊόντων
void initialize_catalog() {
    for (int i = 0; i < NUM_PRODUCTS; i++) {
        snprintf(catalog[i].description, sizeof(catalog[i].description), "Product %d", i);
        catalog[i].price = (i + 1) * 10.0; // Τιμή 10, 20, 30, ...
        catalog[i].item_count = 2; // 2 τεμάχια διαθέσιμα για κάθε προϊόν
    }
}

// Διαδικασία εξυπηρέτησης παραγγελίας
void process_order(int product_id, int pipe_write) {
    // Καθυστέρηση διεκπεραίωσης της παραγγελίας (0.5 δευτ.)
    sleep(1);

    if (product_id >= NUM_PRODUCTS || catalog[product_id].item_count <= 0) {
        // Δεν υπάρχει διαθέσιμο προϊόν
        write(pipe_write, "Order Failed", sizeof("Order Failed"));
    } else {
        // Εξυπηρέτηση παραγγελίας
        catalog[product_id].item_count--;
        char response[100];
        snprintf(response, sizeof(response), "Order Success, Total: %.2f", catalog[product_id].price);
        write(pipe_write, response, strlen(response) + 1); // Παίρνουμε την απάντηση
    }
}

// Εξυπηρέτηση πελάτη
void serve_customer(int pipe_read, int pipe_write) {
    int product_id;
    while (read(pipe_read, &product_id, sizeof(product_id)) > 0) {
        process_order(product_id, pipe_write);
    }
}

// Διαδικασία πελάτη
void customer(int customer_id, int pipe_read, int pipe_write) {
    srand(customer_id); // Για τυχαία επιλογή προϊόντων
    for (int i = 0; i < MAX_ORDER; i++) {
        int product_id = rand() % NUM_PRODUCTS;
        write(pipe_write, &product_id, sizeof(product_id));  // Στέλνουμε την παραγγελία
        sleep(1);  // Ανάμεσα στις παραγγελίες

        char response[100];
        read(pipe_read, response, sizeof(response)); // Διαβάζουμε την απάντηση
        printf("Customer %d: %s\n", customer_id, response);
    }
}

// Κυρίως πρόγραμμα
int main() {
    // Αρχικοποίηση του καταλόγου
    initialize_catalog();

    // Δημιουργία pipes για επικοινωνία με κάθε πελάτη
    int request_pipes[5][2]; // 5 πελάτες, 2 pipes για κάθε πελάτη (1 για αίτημα και 1 για απάντηση)
    int response_pipes[5][2];

    // Δημιουργούμε τα pipes για επικοινωνία
    for (int i = 0; i < 5; i++) {
        if (pipe(request_pipes[i]) == -1 || pipe(response_pipes[i]) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    // Δημιουργία διεργασιών πελατών
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Κώδικας πελάτη
            close(request_pipes[i][0]); // Κλείνουμε το pipe read για αιτήματα
            close(response_pipes[i][1]); // Κλείνουμε το pipe write για απαντήσεις
            customer(i, response_pipes[i][0], request_pipes[i][1]);
            exit(0);
        } else {
            // Κώδικας εξυπηρετητή (πατρική διεργασία)
            close(request_pipes[i][1]); // Κλείνουμε το pipe write για αιτήματα
            close(response_pipes[i][0]); // Κλείνουμε το pipe read για απαντήσεις
            serve_customer(request_pipes[i][0], response_pipes[i][1]);
        }
    }

    // Αναμονή για όλους τους πελάτες
    for (int i = 0; i < 5; i++) {
        wait(NULL);
    }

    return 0;
}

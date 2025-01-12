#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_PRODUCTS 20
#define MAX_ORDER 10
#define NUM_CUSTOMERS 5

//Struct gia ta proionta
typedef struct {
    char description[50];
    float price;
    int item_count;
} Product;

Product catalog[NUM_PRODUCTS];

//Arxikopoiisi counters
int total_requests = 0;
int total_successful = 0;
int total_failed = 0;
int total_products_requested = 0;
int total_products_sold = 0;
float total_revenue = 0.0;

//Initialisation katalogou proionton
void initialize_catalog() {
    const char* product_names[] = {"Stove", "TV", "Speaker", "Washing Machine", "Switch", "Fridge", "And", "Laptop", "Phone"}; //Lista char pou tha pairnei to description
    srand(time(NULL));  

    for (int i = 0; i < NUM_PRODUCTS; i++) {
        //Dimiourgia tixaias perigrafis proionton
        int name_index = rand() % 9;
        snprintf(catalog[i].description, sizeof(catalog[i].description), "%s %d", product_names[name_index]);

        //Random arithmos gia tin timi tou kathe proiontos
        catalog[i].price  = rand() % 50 + 1;  // Price between 1 and 50

        //Arxikopoiisi item_count se 2
        catalog[i].item_count = 2;
    }
}

//Background process kathe ksexoristis paraggelias tou kathe pelati
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

//Eksipiretisi enos pelati
void serve_customer(int pipe_read, int pipe_write) {
    int product_id;
    while (read(pipe_read, &product_id, sizeof(product_id)) > 0) {
        total_products_requested++;               //sum products requested
        total_requests++;                         //sum requests
        if (product_id >= NUM_PRODUCTS || catalog[product_id].item_count <= 0) {
            total_failed++;                                 //sum failed orders
        } else {
            total_successful++;                             //sum successful orders
            total_products_sold++;                          //sum ola ta items sold
            total_revenue += catalog[product_id].price;     //sum olon
        }
        process_order(product_id, pipe_write);
    }
}

//Customer process
void customer(int customer_id, int pipe_read, int pipe_write) {
    srand(customer_id);
    float customer_total = 0.0;  //Sum gia ta total xrimata pou exei dosei o pelatis

    for (int i = 0; i < MAX_ORDER; i++) {
        int product_id;
        
        do {
           product_id = rand() % NUM_PRODUCTS;  //Allazei proion pou thelei na agorasei o pelatis
          } while (catalog[product_id].item_count <= 0);  //An den yparxei to proion
     
          write(pipe_write, &product_id, sizeof(product_id));

        char response[100]; //Apantisi Server se pelati
        read(pipe_read, response, sizeof(response));

        //Sum customer_total
        if (strstr(response, "Order Success") != NULL) {
            float price;
            sscanf(response, "Order Success, Total: %f", &price);
            customer_total += price;
        }

        sleep(1);  //1 second delay metaksi kathe paraggelias pelati
    }

    //Print customer's total spendings
    printf("Client %d: Purchase complete, your total is: %.2f euro.\n", customer_id + 1, customer_total);
}

// Main function
int main() {
    //kalei to initialize_catalog
    initialize_catalog();

    //Pinakes gia tin diadikasia ton pipes
    int request_pipes[NUM_CUSTOMERS][2];  //pipes gia aitima pelati ston "server"
    int response_pipes[NUM_CUSTOMERS][2];   //apantisi "server" piso stous pelates

    //Dimiourgia pipes gia kathe pelati
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if (pipe(request_pipes[i]) == -1 || pipe(response_pipes[i]) == -1) {
            perror("pipe failed");
            exit(EXIT_FAILURE);
        }
    }

    //Fork gia kathe pelati
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } 
        //Epikoinonia metaksi server kai pelati   
        else if (pid == 0) {    //Child (Pelatis)
            close(request_pipes[i][0]);
            close(response_pipes[i][1]);
            customer(i, response_pipes[i][0], request_pipes[i][1]);
            exit(0);
        }
        else {      //Parent (Server)
            close(request_pipes[i][1]);
            close(response_pipes[i][0]);
            serve_customer(request_pipes[i][0], response_pipes[i][1]);
        }
    }
    
    //Anamoni gia na teleiwsoun oi diadikasies metaksi Server kai Pelati
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        wait(NULL);
    }

    //Final Output
    printf("\n%d request were made, where %d succeeded and %d failed\n", 
           total_requests, total_successful, total_failed);
    printf("%d products were requested, where %d products were bought, totaling %.2f euros\n",
           total_products_requested, total_products_sold, total_revenue);

    return 0;
}

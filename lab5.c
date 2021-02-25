
/*
*Colton Rivard
*sec 021
*Lab5 The Farm 
*/


#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct {
    int numfield;
    int numcrop;
    int cropYeildTime;
}field;

struct customer{
    int turnips;
    int radishes;
    int wait;
    int otherCustomers;
    int filledOrder;
    int element;
};

static int radishBin = 0;
static int turnipBin = 0;
int maxradish = 0;
int maxturnip = 0;



pthread_mutex_t consumer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t element_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t has_turnips = PTHREAD_COND_INITIALIZER;
pthread_cond_t has_radishes = PTHREAD_COND_INITIALIZER;

//startFlag
volatile int start = 0;

    //creates a new thread for each field and allows the givin turnip to grow at its yeild time
    void* turnipfield_process(void* args ){
   
   
         field turnipfield = *((field *) args);
        
        	while(!start);
            for(int j = 0; j < turnipfield.numcrop; j++){
                pthread_mutex_lock(&element_mutex);
                turnipBin++;
                if(turnipBin > maxturnip){
                
                maxturnip = turnipBin;
               
                }
                printf("Turnip field %lu added, Turnip number: %d\n", pthread_self(), turnipBin);

                pthread_cond_signal(&has_turnips);
                pthread_mutex_unlock(&element_mutex);
                usleep(turnipfield.cropYeildTime);
            }
            return NULL;
        }
        
        
// creates thread function for farmer grabing radishes and putting them into
	
    void* radishfield_process(void* args ){
     field radishfield = *(( field *) args);
     printf("\n%d %d %d\n", radishfield.numcrop, radishfield.cropYeildTime,radishfield.numfield);
     
     while(!start);
    for(int j = 0; j < radishfield.numcrop; j++){
        pthread_mutex_lock(&element_mutex);
        radishBin++;
        
        if(radishBin > maxradish){
                
                maxradish = radishBin;
               
                }
        printf("radish field %lu added, radish number: %d\n", pthread_self(), radishBin);

        pthread_cond_signal(&has_radishes);
        pthread_mutex_unlock(&element_mutex);
        usleep(radishfield.cropYeildTime);
    }
    return NULL;
}



    void* customer_process(void* args){
       
        
        // casts args to customer array
        struct customer *customer = *((struct customer(*)[]) args);
       
        
        
         // wait for start of master thread
        while(!start);
        //loops through each customer
        for(int i = 0; i < customer->element; i++){

            //locks mutexs for critical section
            pthread_mutex_lock(&consumer_mutex);
            pthread_mutex_lock(&element_mutex);

            //loops through the number of turnips
            for(int j = 0; j < customer->turnips; j ++){
                //waits for more turnips to be birthed
                if(turnipBin == 0){
                    pthread_cond_wait(&has_turnips, &element_mutex);
                    
                }

                //if gets past the wait and still is greedy for turnips
                if(turnipBin == 0){
                	printf("customer turnips: %d radishes: %d num_customers: %d wait: %d", customer->turnips,customer->radishes,customer->otherCustomers, customer->wait);
                    printf("ERROR: Customer %lu removed with no turnips\n", pthread_self());
                    exit(EXIT_FAILURE);
                }
                //buys turnip
                printf("\nthank you for the turnips\n");
                turnipBin--;
                 
            }
            for(int j = 0; j < customer->radishes; j ++){
                //waits for more turnips to be birthed
                if(radishBin == 0 ){
                    pthread_cond_wait(&has_radishes, &element_mutex);
                }

                //if gets past the wait and still is greedy for radishes
                if(radishBin == 0){
                printf("customer turnips: %d radishes: %d num_customers: %d wait: %d", customer->turnips,customer->radishes,customer->otherCustomers, customer->wait);
                    printf("ERROR: Customer %lu removed with no turnips\n", pthread_self());
                    exit(EXIT_FAILURE);
                }
                //buys radish
                printf("\nthank you for the radish\n");
                radishBin--;
                
            }
            
           
            pthread_mutex_unlock(&element_mutex);
            pthread_mutex_unlock(&consumer_mutex);
            usleep(customer->wait);
            customer->filledOrder++;

        }

        return NULL;
    }

    int main(int argc, char *argv[]) {
        int turnip_fields, turnip_time, num_turnips, radish_fields, radish_time, num_radishes, num_customers, turnip_buy, radish_buy, customer_wait;

	//parses farm file
        FILE *farm = fopen(argv[1], "r");
        fscanf(farm, "%d%d%d", &turnip_fields, &turnip_time, &num_turnips);
        fscanf(farm, "%d%d%d", &radish_fields, &radish_time, &num_radishes);
        fscanf(farm, "%d", &num_customers);
        
       

        //initalizes turnip fields
      	 field turnipField;
        turnipField.numfield = turnip_fields;
        turnipField.cropYeildTime = turnip_time;
        turnipField.numcrop = num_turnips;
	
        //initalizes radish feilds
         field radishField;
        radishField.numfield = radish_fields;
        radishField.cropYeildTime = radish_time;
        radishField.numcrop =num_radishes;
        
        
         if(num_customers == 0){
        printf("you didnt get any customers :'(");
        exit(EXIT_FAILURE);
        }
        
        if(radish_fields == 0 && turnip_fields == 0){
        printf("pour harvest :(((((");
        exit(EXIT_FAILURE);
        }  
        
         

        //initializses all the customers
        struct customer customer_arr[num_customers];
        for (int i = 0; i < num_customers; i++) {
            fscanf(farm, "%d%d%d", &turnip_buy, &radish_buy, &customer_wait);
            customer_arr[i].turnips = turnip_buy;
            customer_arr[i].radishes = radish_buy;
            customer_arr[i].wait = customer_wait;
            customer_arr[i].otherCustomers = num_customers;
            customer_arr[i].element = turnip_fields + radish_fields * num_radishes + num_turnips;
            customer_arr[i].filledOrder = 0;
        }
	
	//allocates memory for threads
        pthread_t *turnips = malloc(turnipField.numfield * sizeof(field));
        pthread_t *radishes = malloc(radishField.numfield * sizeof(field));
        pthread_t *customers = malloc(num_customers * sizeof(struct customer));



	//creates threads
        for(int i = 0; i < turnipField.numfield; i++ ){
            if(pthread_create(&turnips[i],NULL,turnipfield_process,&turnipField)== -1){
                printf("COULD NOT CREATE TURNIP PROCESS");
                exit(EXIT_FAILURE);
            }

        }

        for(int i = 0; i < radishField.numfield; i++ ){
            if(pthread_create(&radishes[i],NULL,radishfield_process,&radishField)== -1){
                printf("COULD NOT CREATE RADISH PROCESS");
                exit(EXIT_FAILURE);
            }

        }
	
        for(int i = 0; i < num_customers; i++ ){
            if(pthread_create(&customers[i],NULL,customer_process,&customer_arr[i]) == -1){
                printf("COULD NOT CREATE CUSTOMER PROCESS");
                exit(EXIT_FAILURE);
            }
        }


        start = 1;
        //joins the field threads
        for(int i = 0; i < turnipField.numfield; i++ ){
            pthread_join(turnips[i],NULL);

        }

        for(int i = 0; i < radishField.numfield; i++ ){
            pthread_join(radishes[i], NULL);

        }
        
        
        
        printf("\nTurnip Fields - Number: %d Total: %d Time: %d\n", turnipField.numfield, turnipField.numcrop, turnipField.cropYeildTime); 
        printf("Radish Fields - Number: %d Total: %d Time: %d\n", radishField.numfield, radishField.numcrop, radishField.cropYeildTime); 
        for(int i = 0; i < num_customers; i++){
        printf("Customer %d - Turnips: %d Radishes: %d Wait: %d \n", i, customer_arr[i].turnips, customer_arr[i].radishes,customer_arr[i].wait); 
        }
        printf("\nSimulation Over:\n");
        printf("the max radishes in bin was %d\n",maxradish);
        printf("the max turnips in bin was %d\n",maxturnip);
        for(int i = 0; i < num_customers; i++){
        printf("Customer %d got their order filled %d times\n", i, customer_arr[i].filledOrder);
        }
        
        free(turnips);
        free(radishes);
        free(customers);
         

        
	return EXIT_SUCCESS;
    }



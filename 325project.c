#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#define MAX 100000
int
mark_visited[MAX],customer_priority[MAX],arrival_time[MAX],total_visit_of_customer[MAX],customer_id[MAX],staff_id[MAX];
int
requesting_order=0,customer_done=0,occupied_table=0,total_table,total_customer,total_staff,chair_limit;
sem_t customer,coordinator,served_customer[MAX],mutexLock;
void *customer_thread(void *customer_id)
{
 int c_id=*(int*)customer_id; //converting void to int
 while(1)
 {
 if(total_visit_of_customer[c_id-1] == chair_limit)
 {
 sem_wait(&mutexLock);
 customer_done++; //how many customers have served totally
 sem_post(&mutexLock);
 printf("\n\t customer-%d terminates\n",c_id);
 if(customer_done == total_customer) printf("\n\t All customers Have Recieved Chair\n");
 sem_post(&customer); //notify coordinate to terminate
 pthread_exit(NULL); //thread ends
 }
 sem_wait(&mutexLock); //mutex was initially 1, when we call sem_wait(), it turns 0, it is locked right now
 if(occupied_table == total_table)
 {
 sem_post(&mutexLock); //mutex unlockes it, by turning the value 1
 continue;
 }
 occupied_table++;
 requesting_order++; //request number of customer, it keeps the track of when the customer came
 mark_visited[c_id-1]=requesting_order;
 printf("\nCustomer Thread --> customer-%d takes a Seat.\nCustomer Thread -->Empty Tables = %d\n",c_id,total_table-occupied_table);
 sem_post(&mutexLock); //unlocked for other customers
 sem_post(&customer); //notify coordinator that customer seated.
 sem_wait(&served_customer[c_id-1]); //wait to be serverd.
 printf("\nCustomer Thread --> Customer-%d Received Chair.\n",c_id);
 sem_wait(&mutexLock);
 total_visit_of_customer[c_id-1]++; //tutored times++ after served.
 printf("\nCustomer Thread --> Customer-%d's Priority now is %d\n",c_id,
total_visit_of_customer[c_id-1]);
 sem_post(&mutexLock);
 }
}
void *coordinator_thread()
{
 while(1)
 {
 if(customer_done==total_customer) //if all customers finish, customers threads and coordinate thread terminate.
 {
 for(int i=0; i<total_staff; i++) sem_post(&coordinator); //notify staffs to terminate
 printf("\n\t coordinator terminates\n"); //terminate coordinate itself
 pthread_exit(NULL); //thread ends
 }
 sem_wait(&customer); // waiting for customers signal
 sem_wait(&mutexLock);
 for(int i=0; i<total_customer; i++) //find the customers who just seated and push them into the priority queue
 {
 if(mark_visited[i]>-1)
 {
 customer_priority[i] = total_visit_of_customer[i];
 arrival_time[i] = mark_visited[i]; //to save the time when the customer came
 printf("\nCoordinator Thread --> Customer-%d with Priority-%d in the queue.\n",customer_id[i],total_visit_of_customer[i]);
 mark_visited[i]=-1;
 sem_post(&coordinator); //notify staff that customer is in the queue.
 }
 }
 sem_post(&mutexLock);
 }
}
void *staff_thread(void *staff_id)
{
 int s_id=*(int*)staff_id;
 while(1)
 {
 if(customer_done==total_customer) //if all customers finish, staffs threads terminate.
 {
 sem_wait(&mutexLock);
 printf("\n\t staff-%d terminates\n",s_id);
 sem_post(&mutexLock);
 pthread_exit(NULL); //thread ends
 }
 int max_request=total_customer*chair_limit+1; //this is the maximum serial a customer can get
 int max_priority = chair_limit-1;
 int c_id=-1; //it is a flag.
 sem_wait(&coordinator); //wait coordinator's notification
 sem_wait(&mutexLock);
 for(int i=0; i<total_customer; i++) //find customer with highest priority from priority queue. If customers with same priority, who come first has higher priority
 {
 if(customer_priority[i]>-1 && customer_priority[i]<= max_priority)
 {
 if(arrival_time[i]<max_request)
 {
 max_priority=customer_priority[i];
 max_request=arrival_time[i]; //who comes first, here we are trying to find the minimum arrival time if the priotiy is same
 c_id=customer_id[i];
 }
 }
 }
 if(c_id==-1) //in case no customer in the queue.
 {
 sem_post(&mutexLock);
 continue;
 }
 customer_priority[c_id-1] = -1; //reset the priority queue
 arrival_time[c_id-1] = -1;
 occupied_table--;
 sem_post(&mutexLock);
 sem_wait(&mutexLock); //after tutoring
 printf("\nStaff Thread --> Customer-%d is served by staff-%d\n",c_id,s_id);
 sem_post(&mutexLock);
 sem_post(&served_customer[c_id-1]); //update shared data so customer can know who served him.
 }
}
int main()
{
 printf("Total number of customers: ");
 scanf("%d", &total_customer);
 printf("Total number of staff: ");
 scanf("%d", &total_staff);
 printf("Total number of Tables: ");
 scanf("%d", &total_table);
 printf("Maximum number of chair a customer can get: ");
 scanf("%d", &chair_limit);
 for(int i=0; i<total_customer; i++)
 {
 mark_visited[i]=-1;
 customer_priority[i] = -1;
 arrival_time[i] = -1;
 total_visit_of_customer[i]=0;
 }
 sem_init(&customer,0,0);
 sem_init(&coordinator,0,0);
 sem_init(&mutexLock,0,1);
 for(int i=0; i<total_customer; i++) sem_init(&served_customer[i],0,0);
 pthread_t customers[total_customer],staff[total_staff],coordinator; //allocate threads
 for(int i = 0; i < total_customer; i++) //create threads for customer
 {
 customer_id[i] = i + 1; //saved in array
 pthread_create(&customers[i], NULL, customer_thread, (void*) &customer_id[i]);
 }
 for(int i = 0; i < total_staff; i++) //create threads for staffs
 {
 staff_id[i] = i + 1;
 pthread_create(&staff[i], NULL, staff_thread, (void*) &staff_id[i]);
 }
 pthread_create(&coordinator,NULL,coordinator_thread,NULL); //create thread for coordinator
 //join threads, to connect the threads to main program
 for(int i =0; i < total_customer; i++) pthread_join(customers[i],NULL);
 for(int i =0; i < total_staff; i++) pthread_join(staff[i],NULL);
 pthread_join(coordinator, NULL);
}

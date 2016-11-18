/************************************************************
* Student name and No.: WANG Haicheng 3035140108
* Development platform: Ubuntu 14.04 dual boot with Windows 10
* Last modified date: Nov 11, 2016
* Compilation: gcc DPP.c -o DPP -Wall -pthread
*************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

// define some of constant easy for coding
#define THINKING 1
#define EATIING 2
#define WAITING 3
#define TERMINATED 4
#define GOLDFORK 5
#define SILVERFORK 6
#define FREE -1


// define the struct for better abstraction

/* 
   fork: abstraction of fork, define its type to distinguish 
   so that we could break the circular waiting condition
   also the mutex of the fork is inside of the struct
*/
typedef struct fork
{
	int fork_id;
	int holder;	
	int type;
	pthread_mutex_t use;
}fork;

/*
	philosopher: abstraction of philosopher,
	define its id and status,
	also with left and right fork pointer for easy calling
*/
typedef struct philosopher
{
	pthread_t thread_id;
	int philosopher_id;
	int status;
	fork* left_fork;
	fork* right_fork;
	int eat_times;
}philosopher;

// define this struct in order to pass multiple arguments to philosopher thread
typedef struct philosopher_thread_arg
{
	philosopher* philosopher;
	int min_time;
}philosopher_thread_arg;


// global varibles includes one mutex
philosopher ** philosophers;
fork ** forks;
pthread_mutex_t peek;

/*
	philosopher think function
	argument: philosopher struct containing philosopher info
	return: None
*/
void think(philosopher* philosopher)
{
	// change the status which needs mutex to lock the watcher reading request
	pthread_mutex_lock(&peek);
	philosopher->status = THINKING;
	pthread_mutex_unlock(&peek);

	// sleep random time
	int time_sleep = 1 + random() % (10000000);
	usleep(time_sleep);
}

/*
	philosopher eat function
	argument: philosopher struct containing philosopher info
	return: None
*/
void eat(philosopher* philosopher)
{
	// change the status which needs mutex to lock the watcher reading request
	pthread_mutex_lock(&peek);
	philosopher->status = EATIING;
	philosopher->eat_times +=1;
	pthread_mutex_unlock(&peek);

	// eat random time
	int time_sleep = 1 + random() % (10000000);
	usleep(time_sleep);
}
/*
	philosopher pick up fork function
	argument: philosopher struct containing philosopher info
	return: None
*/
void pick_up_forks(philosopher* philosopher)
{
	// change the status which needs mutex to lock the watcher reading request
	pthread_mutex_lock(&peek);
	philosopher->status = WAITING;
	pthread_mutex_unlock(&peek);
	//  find the position of gold fork
	fork* first;
	fork* second;
	if (philosopher->left_fork->type == GOLDFORK)
	{
		first = philosopher->left_fork;
		second = philosopher->right_fork;
	}
	else
	{
		first = philosopher->right_fork;
		second = philosopher->left_fork;
	}
	// first request to hold the gold fork and then silver fork
	// when successfully get the fork, change holder info of that fork to be philosopher himself
	pthread_mutex_lock(&(first->use));
	pthread_mutex_lock(&peek);
	first->holder = philosopher->philosopher_id;
	pthread_mutex_unlock(&peek);

	pthread_mutex_lock(&(second->use));
	pthread_mutex_lock(&peek);
	second->holder = philosopher->philosopher_id;
	pthread_mutex_unlock(&peek);

}
/*
	philosopher put down fork function
	argument: philosopher struct containing philosopher info
	return: None
*/
void put_down_forks(philosopher* philosopher)
{
	//  find the position of gold fork
	fork* first;
	fork* second;
	if (philosopher->left_fork->type == GOLDFORK)
	{
		first = philosopher->left_fork;
		second = philosopher->right_fork;
	}
	else
	{
		first = philosopher->right_fork;
		second = philosopher->left_fork;
	}
	// first release the gold fork and then silver fork
	// when successfully release the fork, change holder info of that fork to be philosopher himself
	// note that here we must make the whole changing process atomic
	pthread_mutex_lock(&peek);
	pthread_mutex_unlock(&(first->use));
	first->holder =FREE;

	pthread_mutex_unlock(&(second->use));
	second->holder =FREE;
	philosopher->status = THINKING;
	pthread_mutex_unlock(&peek);

}

/*
	philosopher simulate function, the function which philosophers' threads will run
	argument: philosopher struct containing philosopher info
	return: None
*/
void* philosopher_simulate(void* philosopher_thread_arg_void)
{
	// convert to the correct type
	philosopher_thread_arg* thread_arg = (philosopher_thread_arg *) philosopher_thread_arg_void;
	philosopher* philosopher = thread_arg->philosopher;
	int min_time = thread_arg->min_time;
	// get the start time in order to get the exit point
	time_t start_time;
	time_t now;
	time(&start_time);
	// main loop for keeping thinking and eating
	while (1)
	{
		think(philosopher);
		pick_up_forks(philosopher);
		eat(philosopher);
		put_down_forks(philosopher);
		// check if need to exit the thread
		time(&now);
		if ((now - start_time) > min_time)
		{
			break;
		}
	}
	// when exit we change the status
	pthread_mutex_lock(&peek);
	philosopher->status = TERMINATED;
	pthread_mutex_unlock(&peek);
}


/*
	watcher watch function, the function which watcher runs every loop per 0.5 seconds
	argument: number of philosopher
	return: the number of active (non-terminated) philosophers
*/
int watch(int num_of_philosophers)
{
	// print margin and header
	int i;
	for (i = 0; i < 3; i++)
	{
		printf("\n");
	}
	printf("Philo\t%-16sFork\tHeld by\n", "State");

	// initialize the varible for statistics
	int thinking_total = 0;
	int waiting_total = 0;
	int eating_total = 0;
	int use_total = 0;
	int avail_total = 0;
	char state[20];
	char holder[5];
	// get the peek lock to iterate through the philosophers and locks
	// prevent all philosophers and locks status change
	pthread_mutex_lock(&peek);
	for (i = 0; i < num_of_philosophers; i++)
	{
		// check philosophers states
		if (philosophers[i]->status == THINKING)
		{	
			thinking_total ++;
			strcpy(state,"Thinking");
		}
		else if(philosophers[i]->status == WAITING)
		{
			waiting_total ++;
			strcpy(state,"Waiting");
		}
		else if(philosophers[i]->status == EATIING)
		{
			eating_total ++;
			strcpy(state,"Eating");
		}
		else if(philosophers[i]->status == TERMINATED)
		{
			strcpy(state,"Terminated");
		}
		// check forks states
		if(forks[i]->holder != FREE)
		{
			use_total ++;
			sprintf(holder, "%d", forks[i]->holder);
		}
		else
		{
			avail_total ++;
			strcpy(holder,"Free");
		}
		printf("[%2d]:\t%-16s[%2d]:\t%s\n", i,state,i,holder);
	}
	pthread_mutex_unlock(&peek);
	printf("Th=%2d Wa=%2d Ea=%2d\tUse=%2d\tAvail=%2d\n", thinking_total, waiting_total, eating_total, use_total, avail_total);
	int non_terminated = thinking_total + waiting_total + eating_total;
	return non_terminated;
}

/*
	watcher simulate function, the function which watcher thread will run
	argument: number of philosopher
	return: None
*/
void* watcher_simulate(void* num_of_philosophers_void)
{
	//convert to the right type
	int * num_of_philosophers = (int*) num_of_philosophers_void;
	int non_terminated = num_of_philosophers;
	while(1)
	{
		non_terminated = watch(*num_of_philosophers);
		if (non_terminated == 0)
		{
			break;
		}
		usleep(500*1000);
	}
	int i;
	for (i = 0; i < num_of_philosophers; i++)
	{
		printf("philosopher id: %d, eat %d times\n", philosophers[i]->philosopher_id, philosophers[i]->eat_times);
	}
}
/*
	main function
	arguments: three arguments as the specification requiers
	1. number of philosophers
	2. random seed (here we assume it as an integer)
	3. minimum time to run
	return: exit status
*/
int main(int argc, char const *argv[])
{
	// get the three variables from prompt
	int num_of_philosophers = atoi(argv[1]);
	int random_seed = atoi(argv[2]);
	int min_time = atoi(argv[3]);
	// init random seed
	srand(random_seed);
	// prepare the philosophers and forks
	forks = (fork**)malloc(num_of_philosophers*sizeof(fork*));
	philosophers = (philosopher**)malloc(num_of_philosophers*sizeof(philosopher*));
	int i;
	for (i = 0; i < num_of_philosophers; i++ )
	{
		forks[i] = (fork*)malloc(sizeof(fork));
		forks[i]->fork_id = i;
		forks[i]->holder = FREE;
		// we assign the fork whose id is even to be gold fork and other to be silver fork
		if ((i % 2) == 0)
		{
			forks[i]->type = GOLDFORK;
		}
		else
		{
			forks[i]->type = SILVERFORK;
		}
	}
	// initiate the philosopher list
	// find its right fork and left fork
	for (i = 0; i < num_of_philosophers; i++)
	{
		philosophers[i] = (philosopher*)malloc(sizeof(philosopher));
		philosophers[i]->philosopher_id = i;
		// philosophers[i]->status
		int left_fork_index = (i - 1 + num_of_philosophers) % num_of_philosophers;
		int right_fork_index = i;
		philosophers[i]->left_fork = forks[left_fork_index];
		philosophers[i]->right_fork = forks[right_fork_index];
		philosophers[i]->eat_times = 0;
	}

	pthread_t watcher;
	// create threads for philosophers
	for (i = 0; i < num_of_philosophers; i++)
	{
		philosopher_thread_arg* thread_arg = (philosopher_thread_arg*)malloc(sizeof(philosopher_thread_arg));
		thread_arg->philosopher = philosophers[i];
		thread_arg->min_time = min_time;
		if(pthread_create(&(philosophers[i]->thread_id), NULL, philosopher_simulate, thread_arg)) 
		{
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}
	// create threads for the watcher
	if(pthread_create(&watcher, NULL, watcher_simulate, &num_of_philosophers)) 
	{
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	// wait threads
	for (i = 0; i < num_of_philosophers; i++)
	{
		pthread_join(philosophers[i]->thread_id, NULL);
	}
	pthread_join(watcher, NULL);

	return 0;
}
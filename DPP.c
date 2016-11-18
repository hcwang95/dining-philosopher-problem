#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define THINKING 1
#define EATIING 2
#define WAITING 3
#define TERMINATED 4
#define GOLDFORK 5
#define SILVERFORK 6
#define FREE -1

// this is only valid for debug
#define DEBUG 0
#define TIMEBUG 1


typedef struct fork
{
	int fork_id;
	int holder;	
	int type;
	pthread_mutex_t use;
}fork;

typedef struct philosopher
{
	pthread_t thread_id;
	int philosopher_id;
	int status;
	fork* left_fork;
	fork* right_fork;
}philosopher;

typedef struct philosopher_thread_arg
{
	philosopher* philosopher;
	int random_seed;
	int min_time;
}philosopher_thread_arg;

// global varibles includes mutex
philosopher ** philosophers;
fork ** forks;
pthread_mutex_t peek;


void think(philosopher* philosopher, int random_seed)
{
	if (DEBUG)
	{
		printf("thinking id : %d\n", philosopher->philosopher_id);
	}
	pthread_mutex_lock(&peek);
	philosopher->status = THINKING;
	if (DEBUG)
	{
		printf("%d id : %d\n",philosopher->status,  philosopher->philosopher_id);
	}
	pthread_mutex_unlock(&peek);
	srand(random_seed);
	int time_sleep = 1+ random() % (10000000);
	if (TIMEBUG)
	{
		printf("%d\n", time_sleep);
	}
	usleep(time_sleep);
}

void eat(philosopher* philosopher, int random_seed)
{
	if (DEBUG)
	{
		printf("eating id : %d\n", philosopher->philosopher_id);
	}
	pthread_mutex_lock(&peek);
	philosopher->status = EATIING;
	if (DEBUG)
	{
		printf("%d id : %d\n",philosopher->status,  philosopher->philosopher_id);
	}
	pthread_mutex_unlock(&peek);
	srand(random_seed);
	int time_sleep = 1+ random() % (10000000);
	if (TIMEBUG)
	{
		printf("%d\n", time_sleep);
	}
	usleep(time_sleep);
}

void pick_up_forks(philosopher* philosopher)
{
	if (DEBUG)
	{
		printf("pick_up_forks: id=%d\n", philosopher->philosopher_id);
	}
	// register to be waiting / hungury
	pthread_mutex_lock(&peek);
	philosopher->status = WAITING;
	if (DEBUG)
	{
		printf("%d id : %d\n",philosopher->status,  philosopher->philosopher_id);
	}
	pthread_mutex_unlock(&peek);
	fork* first;
	fork* second;
	//  check if the position of gold fork
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
	pthread_mutex_lock(&(first->use));
	pthread_mutex_lock(&peek);
	first->holder = philosopher->philosopher_id;
	pthread_mutex_unlock(&peek);
	pthread_mutex_lock(&(second->use));
	pthread_mutex_lock(&peek);
	second->holder = philosopher->philosopher_id;
	pthread_mutex_unlock(&peek);

}

void put_down_forks(philosopher* philosopher)
{
	if (DEBUG)
	{
		printf("put_down_forks: id=%d\n", philosopher->philosopher_id);
	}
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
	pthread_mutex_unlock(&(first->use));
	pthread_mutex_lock(&peek);
	first->holder =FREE;
	pthread_mutex_unlock(&peek);
	pthread_mutex_unlock(&(second->use));
	pthread_mutex_lock(&peek);
	second->holder =FREE;
	pthread_mutex_unlock(&peek);
}

void* philosopher_simulate(void* philosopher_thread_arg_void)
{
	// convert to the correct type
	philosopher_thread_arg* thread_arg = (philosopher_thread_arg *) philosopher_thread_arg_void;
	philosopher* philosopher = thread_arg->philosopher;
	int random_seed = thread_arg->random_seed;
	int min_time = thread_arg->min_time;
	// get the start time
	time_t start_time;
	time_t now;
	time(&start_time);
	while (1)
	{
		if (DEBUG)
		{
			printf("active\n");
		}
		think(philosopher, random_seed);
		pick_up_forks(philosopher);
		eat(philosopher, random_seed);
		put_down_forks(philosopher);
		time(&now);
		if ((now - start_time) > min_time)
		{
			break;
		}
	}
	pthread_mutex_lock(&peek);
	philosopher->status = TERMINATED;
	pthread_mutex_unlock(&peek);
}



int watch(int num_of_philosophers)
{
	int i;
	for (i = 0; i < 3; i++)
	{
		printf("\n");
	}
	printf("Philo\t%-16sFork\tHeld by\n", "State");
	int thinking_total = 0;
	int waiting_total = 0;
	int eating_total = 0;
	int use_total = 0;
	int avail_total = 0;
	char state[20];
	char holder[5];
	pthread_mutex_lock(&peek);
	for (i = 0; i < num_of_philosophers; i++)
	{
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
		if (DEBUG)
		{
			printf("%s id : %d\n",state,  philosophers[i]->philosopher_id);
		}
		printf("[%2d]:\t%-16s[%2d]:\t%s\n", i,state,i,holder);
	}
	pthread_mutex_unlock(&peek);
	printf("Th=%2d Wa=%2d Ea=%2d\tUse=%2d\tAvail=%2d\n", thinking_total, waiting_total, eating_total, use_total, avail_total);
	int non_terminated = thinking_total + waiting_total + eating_total;
	return non_terminated;
}
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
}
int main(int argc, char const *argv[])
{
	// get the three variables from prompt
	int num_of_philosophers = atoi(argv[1]);
	int random_seed = atoi(argv[2]);
	int min_time = atoi(argv[3]);
	if (DEBUG)
	{
		fprintf(stdout, "%d%d%d\n", num_of_philosophers,random_seed,min_time);
	}

	// prepare the philosophers and forks
	forks = (fork**)malloc(num_of_philosophers*sizeof(fork*));
	philosophers = (philosopher**)malloc(num_of_philosophers*sizeof(philosopher*));
	int i;
	for (i = 0; i < num_of_philosophers; i++ )
	{
		forks[i] = (fork*)malloc(sizeof(fork));
		forks[i]->fork_id = i;
		forks[i]->holder = FREE;
		if ((i % 2) == 0)
		{
			forks[i]->type = GOLDFORK;
		}
		else
		{
			forks[i]->type = SILVERFORK;
		}
	}
	for (i = 0; i < num_of_philosophers; i++)
	{
		philosophers[i] = (philosopher*)malloc(sizeof(philosopher));
		philosophers[i]->philosopher_id = i;
		// philosophers[i]->status
		int left_fork_index = (i - 1 + num_of_philosophers) % num_of_philosophers;
		int right_fork_index = i;
		philosophers[i]->left_fork = forks[left_fork_index];
		philosophers[i]->right_fork = forks[right_fork_index];
		if (DEBUG)
		{
			printf("philosopher %d, left_fork_index:%d, right_fork_index:%d\n", 
				i, left_fork_index, right_fork_index);
		} 
	}
	pthread_t watcher;
	// create threads for philosophers
	for (i = 0; i < num_of_philosophers; i++)
	{
		philosopher_thread_arg* thread_arg = (philosopher_thread_arg*)malloc(sizeof(philosopher_thread_arg));
		thread_arg->philosopher = philosophers[i];
		thread_arg->random_seed = random_seed;
		thread_arg->min_time = min_time;
		if(pthread_create(&(philosophers[i]->thread_id), NULL, philosopher_simulate, thread_arg)) 
		{
			fprintf(stderr, "Error creating thread\n");
			return 1;
		}
	}
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
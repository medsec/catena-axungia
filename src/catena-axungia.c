#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <limits.h>
#include <float.h>
#include <time.h>
#include <math.h>
#include <errno.h>

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define STARTGARLIC 15
#define H_PER_KIB 	16	//1024/H_LEN

//Limits
#define TIME_HIGH 86400
 //Lower limit max_memory is 1KiB which is roughly garlic=4
 //Upper limit for max_memory is given by integer => garlic~35
#define MEMORY_HIGH INT_MAX
#define MEMORY_LOW 1
#define HARD_HIGH 255
#define HARD_DEFAULT 2
#define HARD_LOW 1
#define ITER_HIGH INT_MAX
#define ITER_DEFAULT 3
#define ITER_LOW 1


//functions defined from wrapper.c
extern void catena_BRG(const uint8_t lambda, const uint8_t garlic);
extern void catena_BRGFH(const uint8_t lambda, const uint8_t garlic);
extern void catena_DBG(const uint8_t lambda, const uint8_t garlic);
extern void catena_DBGFH(const uint8_t lambda, const uint8_t garlic);

//Constraints
static int max_time_set = 0;
static double max_time; 	//time in s
static int max_memory_set = 0;
static int max_memory;	//memory in KiB
static int min_mhard = HARD_DEFAULT;		
static int iterations = ITER_DEFAULT;	//Number of runs
static int full_hash = 0;			//use fullhash or not
static int verbose = 0;

void print_welcome(FILE* dest){
	fputs("This application searches for the optimal Catena-Butterfly ",dest);
	fputs("and Catena-Dragonfly parameters for given constraints. Make ",dest);
	fputs("sure to run this under realistic conditions on the most ", dest);
	fputs("constrained system of the ones involved.\n", dest);
}


void print_usage(char **argv){
	fprintf(stderr,"Usage: %s --max_time TIME --max_memory MEMORY ",argv[0]);
	fputs("[--min_mhard\n HARDNESS] [--iterations ITERATIONS] ", stderr);
	fputs("[--full_hash] [--verbose]\n", stderr);
	
	fputs("\n", stderr);

	fprintf(stderr, "-t, --max_time TIME");
	fprintf(stderr, "\tUpper bound for expected password hashing time in \n");
	fprintf(stderr, "\t\t\tseconds(floating point). Max: %d\n",TIME_HIGH );

	fprintf(stderr,"-m, --max_memory MEMORY");
	fprintf(stderr, "\tUpper bound for memory usage in KiB. Min: %d,\n"
			,MEMORY_LOW);
	fprintf(stderr, "\t\t\tMax: %d\n", MEMORY_HIGH);

	fprintf(stderr,"-h, --min_mhard HARDNESS");
	fprintf(stderr,"\tLower bound for memory hardness factor.\n");
	fprintf(stderr,"\t\t\t\tDefault: %d, Min: %d, Max: %d\n", HARD_DEFAULT, 
			HARD_LOW, HARD_HIGH);
	
	fprintf(stderr,"-i, --iterations ITERATIONS");
	fprintf(stderr,"\tNumber of iterations used to determine the\n", stderr);
	fprintf(stderr,"\t\t\t\truntime. Higher values increase stability.\n");
	fprintf(stderr,"\t\t\t\tDefault: %d, Min: %d, Max: %d\n",ITER_DEFAULT, 
			ITER_LOW, ITER_HIGH);
	
	fprintf(stderr,"-f, --full_hash\tUses a full hash function instead of a ");
	fprintf(stderr,"reduced one for\n\t\tconsecutive calls\n");
	
	fprintf(stderr,"-v, --verbose\tOutputs progress information\n");
}


int chkparams(void){
    //check for required parameters
	if(!max_time_set || !max_memory_set){
    	fputs("\nmax_time and max_memory are both required!\n\n", stderr);
    	return 1;
    }
    return 0;
}


int parse_args(int argc, char **argv)
{
	while (1)
	{
		static struct option long_options[] =
		{
		  {"max_time",	required_argument, 	0, 't'},
		  {"max_memory",required_argument, 	0, 'm'},
		  {"min_mhard",	required_argument, 	0, 'h'},
		  {"iterations",required_argument, 	0, 'i'},
		  {"full_hash",	no_argument, 		0, 'f'},
		  {"verbose",	no_argument, 		0, 'v'},
		  /*The last element of the array has to be filled with zeros.*/
		  {0, 			0, 					0, 	0}
		};

		int r; //return value of getopt_long is the corresponding val
		char* endptr = NULL; //for parsing numbers
		long temp;
		double tempd;
		errno = 0;

		//_only also recognizes long options that start with a single -
		r = getopt_long_only(argc, argv, "t:m:h:i:fv", long_options, NULL);

		/* Detect the end of the options. */
		if (r == -1)
			break;

		switch (r)
		{
			case 't':
				tempd = strtod(optarg, &endptr);
			  	if (*endptr != '\0') {
					fputs("argument for max_time could not be parsed\n",
						stderr);	
					return 1;
				}
			    if(tempd <= 0 || tempd > TIME_HIGH || errno == ERANGE){
			    	fputs("max_time out of range\n", stderr);
			    	return 1;
			    }
			    max_time_set = 1;
				max_time = tempd;
			  	break;

			case 'm':
				temp = strtol(optarg, &endptr, 10);
				if (*endptr != '\0') {
					fputs("argument for max_memory could not be parsed\n",
						stderr);
				return 1;
				}
				if(temp < MEMORY_LOW || temp > MEMORY_HIGH || errno == ERANGE){
					fputs("max_memory out of range\n", stderr);
			    	return 1;
			    }
			    max_memory_set = 1;
				max_memory = temp;
				break;

			case 'h':
				temp = strtol(optarg, &endptr, 10);
				if (*endptr != '\0') {
					fputs("argument for min_mhard could not be parsed \n",
						stderr);
					return 1;	
				}
				if(temp < HARD_LOW || temp > HARD_HIGH || errno == ERANGE){
					fputs("min_mhard out of range\n", stderr);
			    	return 1;
			    }
				min_mhard = temp;
				break;

			case 'i':
				temp = strtol(optarg, &endptr, 10);
				if (*endptr != '\0') {
					fputs("argument for iterations could not be parsed \n",
						stderr);
					return 1;	
				}
				if(temp < ITER_LOW || temp > ITER_HIGH || errno == ERANGE){
					fputs("iterations out of range\n", stderr);
			    	return 1;
			    }
				iterations = temp;
				break;

			case 'f':
				full_hash = 1;
				break;

			case 'v':
				verbose = 1;
				break;
		}
	}

	//getopt already informs about unrecognized options
	if (optind < argc)
    {
    	fputs("Some arguments could not be assigned to an option \n", stderr);
    	return 1;
    }

    return chkparams();
}

//taken from libc documentation
int compare_doubles (const void *a, const void *b)
{
  const double *da = (const double *) a;
  const double *db = (const double *) b;

  return (*da > *db) - (*da < *db);
}


double measure(int DBG, uint8_t lambda, uint8_t garlic){
	if(verbose){
		printf(".");
		fflush(stdout);
	}

	double t[iterations];
	clock_t diff;
	
	for(int i = 0; i < iterations; i++){
		diff = clock();
		if(DBG){
			if(full_hash){
				catena_DBGFH(lambda, garlic);
			}
			else{
				catena_DBG(lambda, garlic);
			}
		}
		else{
			if(full_hash){
				catena_BRGFH(lambda,garlic);
			}
			else{
				catena_BRG(lambda, garlic);
			}
		}
	
		diff = clock() - diff;
		t[i] = ((double)diff) / CLOCKS_PER_SEC;
	}
	qsort(t, iterations, sizeof(t[0]), compare_doubles);
	
	return t[iterations/2]; //median
}


void print_result(int DBG, uint8_t g, uint8_t l, double t){
	puts("");
	if(t <= max_time){
		int memory;
		if(DBG){
			memory = (1<<g) * 1.5;
			puts("The recommended parameters for Catena-Butterfly are:");
		}
		else{
			memory = 1<<g;
			puts("The recommended parameters for Catena-Dragonfly are:");
		}

		printf("Garlic: %u\n", g);
		printf("Lambda: %u\n", l);
		if(full_hash){
			puts("with the FULLHASH option enabled");
		}
		printf("This requires %.2fs and ", t);
		printf("slightly more than %.1fKiB of memory\n",memory*(1.0/H_PER_KIB));


		if(l == 1 || DBG){
			printf("The resulting memory hardness is: %u\n", l);
		}
		else if(l > 1 && !DBG){ 
			printf("The resulting memory hardness won't exceed 1\n");
		}
	}
	else{
		printf("No suitable parameters could be found ");
		if(DBG){
			printf("for Catena-Butterfly ");
		}
		else{
			printf("for Catena-Dragonfly ");
		}

		if(full_hash){
			printf("with the FULLHASH option enabled");
		}
		puts("");
	}
}


void search(int DBG){
	uint8_t lambda = min_mhard;

	//BRG requires 2^g memory
	uint8_t max_garlic = (uint8_t)log2(max_memory * H_PER_KIB);
	//DBG requires 1.5 x 2^g memory
	if(DBG && ((1<<max_garlic) * 1.5) > (max_memory * H_PER_KIB)){
		max_garlic--;
	}
	
	//reasonable start garlic
	uint8_t curgarlic = MIN(((double)max_garlic)/4.0, STARTGARLIC);
	curgarlic = MAX(curgarlic, 1); //in case maxgarlic < 4

	double ct = measure(DBG, lambda, curgarlic);
	//start by increasing the memory
	while(curgarlic < max_garlic && ct <= (((double)max_time)/2.0) ){
		curgarlic++;
		ct = measure(DBG, lambda, curgarlic);
	}

	//slowdown from garlic++ is actually bigger than 2
	if(ct > max_time && curgarlic > 1){
		curgarlic--;
		ct = measure(DBG, lambda, curgarlic);
	}

	//try increase lambda if possible twice because increasing lambda has below 
	//linear impact on the runtime due to missing overhead
	for(int i=0; i < 2; i++){
		double tpl = ct/lambda;
		double tleft = max_time - ct;
		int linc = MIN(tleft/tpl, (HARD_HIGH-lambda)); //restrict lambda to 255
		
		if(linc > 0){ //do not decrease lambda!
			lambda += linc;
			ct = measure(DBG, lambda, curgarlic);
		}
		else{
			break;
		}
	}

	//for edge cases where lambda was increased too far
	while(ct > max_time && lambda > min_mhard){
		lambda --;
		ct = measure(DBG, lambda, curgarlic);	
	}

	print_result(DBG, curgarlic, lambda, ct);
}


int main(int argc, char **argv){
	print_welcome(stdout);

	if(parse_args(argc,argv)){
		print_usage(argv);
		return 1;
	}

	search(0); //search BRG parameters
	search(1); //search for DBG parameters

	return 0;
}
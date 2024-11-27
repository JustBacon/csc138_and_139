/*
    Name: Christian Buco

    Linux OS running as virtual machine under Windows
    WSL

    Architecture:             x86_64
    CPU op-mode(s):         32-bit, 64-bit
    Address sizes:          46 bits physical, 48 bits virtual
    Byte Order:             Little Endian
    CPU(s):                   16
    On-line CPU(s) list:    0-15
    Vendor ID:                GenuineIntel
    Model name:             12th Gen Intel(R) Core(TM) i5-12600K
        CPU family:           6
        Model:                151
        Thread(s) per core:   2
        Core(s) per socket:   8
        Socket(s):            1
        Stepping:             2
        BogoMIPS:             7372.80
        Flags:                fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good nopl xtopology tsc_reliable nons
                            top_tsc cpuid pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch invpcid_single ssbd
                            ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves a
                            vx_vnni umip waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize flush_l1d arch_capabilities
    Virtualization features:
    Virtualization:         VT-x
    Hypervisor vendor:      Microsoft
    Virtualization type:    full
    Caches (sum of all):
    L1d:                    384 KiB (8 instances)
    L1i:                    256 KiB (8 instances)
    L2:                     10 MiB (8 instances)
    L3:                     20 MiB (1 instance)
    Vulnerabilities:
    Gather data sampling:   Not affected
    Itlb multihit:          Not affected
    L1tf:                   Not affected
    Mds:                    Not affected
    Meltdown:               Not affected
    Mmio stale data:        Not affected
    Reg file data sampling: Vulnerable: No microcode
    Retbleed:               Mitigation; Enhanced IBRS
    Spec rstack overflow:   Not affected
    Spec store bypass:      Mitigation; Speculative Store Bypass disabled via prctl and seccomp
    Spectre v1:             Mitigation; usercopy/swapgs barriers and __user pointer sanitization
    Spectre v2:             Mitigation; Enhanced / Automatic IBRS; IBPB conditional; RSB filling; PBRSB-eIBRS SW sequence; BHI BHI_DIS_S
    Srbds:                  Not affected
    Tsx async abort:        Not affected
*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/timeb.h>
#include <semaphore.h>
#include <stdbool.h> // For bool, true, false

#define MAX_SIZE 100000000
#define MAX_THREADS 16
#define RANDOM_SEED 7649
#define MAX_RANDOM_NUMBER 3000
#define NUM_LIMIT 9973

// Global variables
long gRefTime; //For timing
int gData[MAX_SIZE]; //The array that will hold the data

int gThreadCount; //Number of threads
int gDoneThreadCount; //Number of threads that are done at a certain point. Whenever a thread is done, it increments this. Used with the semaphore-based solution
int gThreadProd[MAX_THREADS]; //The modular product for each array division that a single thread is responsible for
volatile bool gThreadDone[MAX_THREADS]; //Is this thread done? Used when the parent is continually checking on child threads

// Semaphores
sem_t completed; //To notify parent that all threads have completed or one of them found a zero
sem_t mutex; //Binary semaphore to protect the shared variable gDoneThreadCount

// Function prototypes
int SqFindProd(int size); //Sequential FindProduct (no threads) computes the product of all the elements in the array mod NUM_LIMIT
void* ThFindProd(void* param); //Thread FindProduct but without semaphores
void* ThFindProdWithSemaphore(void* param); //Thread FindProduct with semaphores
int ComputeTotalProduct(); // Multiply the division products to compute the total modular product
void InitSharedVars(); //Initialize shared variables
void GenerateInput(int size, int indexForZero); //Generate the input array
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]); //Calculate the indices to divide the array into T divisions, one division per thread
int GetRand(int min, int max); //Get a random number between min and max

//Timing functions
long GetMilliSecondTime(struct timeb timeBuf);
long GetCurrentTime(void);
void SetTime(void);
long GetTime(void);

int main(int argc, char* argv[]) {
    pthread_t tid[MAX_THREADS];
    pthread_attr_t attr[MAX_THREADS];
    int indices[MAX_THREADS][3];
    int i, indexForZero, arraySize, prod;

    // Code for parsing and checking command-line arguments
    if (argc != 4) {
        fprintf(stderr, "Invalid number of arguments!\n");
        exit(-1);
    }

    if ((arraySize = atoi(argv[1])) <= 0 || arraySize > MAX_SIZE) {
        fprintf(stderr, "Invalid Array Size\n");
        exit(-1);
    }
    gThreadCount = atoi(argv[2]);

    if (gThreadCount > MAX_THREADS || gThreadCount <= 0) {
        fprintf(stderr, "Invalid Thread Count\n");
        exit(-1);
    }

    indexForZero = atoi(argv[3]);

    if (indexForZero < -1 || indexForZero >= arraySize) {
        fprintf(stderr, "Invalid index for zero!\n");
        exit(-1);
    }

    GenerateInput(arraySize, indexForZero);
    CalculateIndices(arraySize, gThreadCount, indices);

    // Code for the sequential part
    SetTime();
    prod = SqFindProd(arraySize);
    printf("Sequential multiplication completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Threaded with parent waiting for all child threads
    InitSharedVars();
    SetTime();

    // Initialize threads, create threads
    // The thread start function is ThFindProd
    for (i = 0; i < gThreadCount; i++) {
        pthread_attr_init(&attr[i]);
        pthread_create(&tid[i], &attr[i], ThFindProd, indices[i]);
    }

    // let the parent wait for all threads using pthread_join
    for (i = 0; i < gThreadCount; i++) {
        pthread_join(tid[i], NULL);
    }

    prod = ComputeTotalProduct();
    printf("Threaded multiplication with parent waiting for all children completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Multi-threaded with busy waiting (parent continually checking on child threads without using semaphores)
    InitSharedVars();
    SetTime();

    // Initialize threads, create threads, and then make the parent continually check on all child threads
    // The thread start function is ThFindProd
    for (i = 0; i <gThreadCount; i++) {
        pthread_attr_init(&attr[i]);
        pthread_create(&tid[i], &attr[i], ThFindProd, indices[i]);
    }

    bool allDone = false;
    while (!allDone) {
        allDone = true;
        for (i = 0; i < gThreadCount; i++) {
            if (!gThreadDone[i]) {
                allDone = false;
                break;
            }
        }
    }

    prod = ComputeTotalProduct();
    printf("Threaded multiplication with parent continually checking on children completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Multi-threaded with semaphores
    InitSharedVars();
    // Initialize your semaphores here
    sem_init(&completed, 0, 0);
    sem_init(&mutex, 0, 1);
    SetTime();

    // Initialize threads, create threads, and then make the parent wait on the "completed" semaphore
    // The thread start function is ThFindProdWithSemaphore
    for (i = 0; i < gThreadCount; i++) {
        pthread_attr_init(&attr[i]);
        pthread_create(&tid[i], &attr[i], ThFindProdWithSemaphore, indices[i]);
    }

    sem_wait(&completed);

    prod = ComputeTotalProduct();
    printf("Threaded multiplication with parent waiting on a semaphore completed in %ld ms. Product = %d\n", GetTime(), prod);

    // Cleanup semaphores
    sem_destroy(&completed);
    sem_destroy(&mutex);
    return 0;
}

// Write a regular sequential function to multiply all the elements in gData mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
int SqFindProd(int size) {
    int prod = 1;

    for (int i =0; i < size; i++) {
        if (gData[i] == 0) {
            return 0;
        }
        prod = (prod * gData[i]) % NUM_LIMIT;
    }

    return prod;
}

// Write a thread function that computes the product of all the elements in one division of the array mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
// When it is done, this function should store the product in gThreadProd[threadNum] and set gThreadDone[threadNum] to true
void* ThFindProd(void* param) {
    int* indices = (int*) param;
    int threadNum = indices[0];
    int start = indices[1];
    int end = indices[2];

    int prod = 1;

    for (int i = start; i <= end; i++) {
        if (gData[i] == 0) {
            gThreadProd[threadNum] = 0;
            gThreadDone[threadNum] = true;
            pthread_exit(0);
        }
        prod = (prod * gData[i]) % NUM_LIMIT;
    }

    gThreadProd[threadNum] = prod;
    gThreadDone[threadNum] = true;

    pthread_exit(0);
}

// Write a thread function that computes the product of all the elements in one division of the array mod NUM_LIMIT
// REMEMBER TO MOD BY NUM_LIMIT AFTER EACH MULTIPLICATION TO PREVENT YOUR PRODUCT VARIABLE FROM OVERFLOWING
// When it is done, this function should store the product in gThreadProd[threadNum]
// If the product value in this division is zero, this function should post the "completed" semaphore
// If the product value in this division is not zero, this function should increment gDoneThreadCount and
// post the "completed" semaphore if it is the last thread to be done
// Don't forget to protect access to gDoneThreadCount with the "mutex" semaphore
void* ThFindProdWithSemaphore(void* param) {
    int* indices = (int*) param;
    int threadNum = indices[0];
    int start = indices[1];
    int end = indices[2];

    int prod = 1;

    for (int i = start; i <= end; i++) {
        if (gData[i] == 0) {
            gThreadProd[threadNum] = 0;
            sem_post(&completed);
            pthread_exit(0);
        }
        prod = (prod * gData[i]) % NUM_LIMIT;
    }

    gThreadProd[threadNum] = prod;

    sem_wait(&mutex);
    gDoneThreadCount++;
    if (gDoneThreadCount == gThreadCount) {
        sem_post(&completed);
    }
    sem_post(&mutex);

    pthread_exit(0);
}

int ComputeTotalProduct() {
    int i, prod = 1;

    for (i = 0; i < gThreadCount; i++) {
        prod *= gThreadProd[i];
        prod %= NUM_LIMIT;
    }

    return prod;
}

void InitSharedVars() {
    int i;

    for (i = 0; i < gThreadCount; i++) {
        gThreadDone[i] = false;
        gThreadProd[i] = 1;
    }
    gDoneThreadCount = 0;
}

// Write a function that fills the gData array with random numbers between 1 and MAX_RANDOM_NUMBER
// If indexForZero is valid and non-negative, set the value at that index to zero
void GenerateInput(int size, int indexForZero) {
    srand(RANDOM_SEED);

    for (int i = 0; i < size; i++) {
        gData[i] = GetRand(1, MAX_RANDOM_NUMBER);
    }

    if (indexForZero >= 0) {
        gData[indexForZero] = 0;
    }
}

// Write a function that calculates the right indices to divide the array into thrdCnt equal divisions
// For each division i, indices[i][0] should be set to the division number i,
// indices[i][1] should be set to the start index, and indices[i][2] should be set to the end index
void CalculateIndices(int arraySize, int thrdCnt, int indices[MAX_THREADS][3]) {
    int chunkSize = arraySize / thrdCnt;
    int remainder = arraySize % thrdCnt;

    for (int i = 0; i < thrdCnt; i++) {
        indices[i][0] = i;
        indices[i][1] = i * chunkSize;
        indices[i][2] = (i == thrdCnt - 1) ? (arraySize - 1) : (indices[i][1] + chunkSize - 1);
    }
}

// Get a random number in the range [x, y]
int GetRand(int x, int y) {
    int r = rand();
    r = x + r % (y - x + 1);
    return r;
}

long GetMilliSecondTime(struct timeb timeBuf) {
    long mliScndTime;
    mliScndTime = timeBuf.time;
    mliScndTime *= 1000;
    mliScndTime += timeBuf.millitm;
    return mliScndTime;
}

long GetCurrentTime(void) {
    long crntTime = 0;
    struct timeb timeBuf;
    ftime(&timeBuf);
    crntTime = GetMilliSecondTime(timeBuf);
    return crntTime;
}

void SetTime(void) {
    gRefTime = GetCurrentTime();
}

long GetTime(void) {
    long crntTime = GetCurrentTime();
    return (crntTime - gRefTime);
}

#include "evaluation_manager.h"

EvaluationManager::EvaluationManager() : manager(false) {
    // Create shared memory mapping
    int flow_fd = shm_open(EVALUATION_FLOW_SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(flow_fd, sizeof(EvaluationQueue));
    this->queue = static_cast<EvaluationQueue*>(mmap(0, sizeof(EvaluationQueue), PROT_READ | PROT_WRITE, MAP_SHARED, flow_fd, 0));
    close(flow_fd);
    // Create locks to regulate access to this manager
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(this->queue->request_lock), &mutex_attr);
    pthread_mutex_init(&(this->queue->results_lock), &mutex_attr);
    pthread_mutexattr_destroy(&mutex_attr);
}

EvaluationManager::~EvaluationManager() {
    if (this->manager) {
        // Perform the final application completion logic
        pthread_mutex_destroy(&(this->queue->request_lock));
        pthread_mutex_destroy(&(this->queue->results_lock));
    }
    // Unmap the shared memory from current process's address space
    munmap(this->queue, sizeof(EvaluationQueue));
    if (this->manager) {
        shm_unlink(EVALUATION_FLOW_SHARED_MEMORY_NAME);
    }
}

// Processes an evaluation request from the trainer
bool EvaluationManager::sendRequest(const double c[C_LENGTH], int seed, int id) {
    bool submitted = false;
    pthread_mutex_lock(&(this->queue->request_lock));
    if (this->queue->request_queue_len < MAX_EVALUATION_QUEUE_LENGTH) {
        int i = this->queue->request_queue_len;
        copy(c, c + C_LENGTH, this->queue->request_queue[i]);
        this->queue->request_queue[i][C_LENGTH] = seed;
        this->queue->request_queue[i][C_LENGTH + 1] = id;
        this->queue->request_queue_len++;
        submitted = true;
    }
    pthread_mutex_unlock(&(this->queue->request_lock));
    return submitted;
}

// Gets an evaluation request for an evaluator to get metrics for
pair<int, int> EvaluationManager::getRequest(double c[C_LENGTH]) {
    pair<int, int> extra_info(-1, -1);
    pthread_mutex_lock(&(this->queue->request_lock));
    if (this->queue->request_queue_len > 0) {
        int i = this->queue->request_queue_len - 1;
        copy(this->queue->request_queue[i], this->queue->request_queue[i] + C_LENGTH, c);
        extra_info = make_pair(this->queue->request_queue[i][C_LENGTH], this->queue->request_queue[i][C_LENGTH + 1]);
        this->queue->request_queue_len--;
    }
    pthread_mutex_unlock(&(this->queue->request_lock));
    return extra_info;
}

// The trainer gets the metrics of a request that has completed evaluation
int EvaluationManager::getResult(double results[EVALUATION_METRICS]) {
    int id = -1;
    pthread_mutex_lock(&(this->queue->results_lock));
    if (this->queue->results_queue_len > 0) {
        int i = this->queue->results_queue_len - 1;
        copy(this->queue->results_queue[i], this->queue->results_queue[i] + EVALUATION_METRICS, results);
        id = this->queue->results_queue[i][EVALUATION_METRICS];
        this->queue->results_queue_len--;
    }
    pthread_mutex_unlock(&(this->queue->results_lock));
    return id;
}

// Once the evaluator finishes the evaluation, it sends the results to the queue
bool EvaluationManager::sendResult(int id, double results[EVALUATION_METRICS]) {
    bool submitted = false;
    pthread_mutex_lock(&(this->queue->results_lock));
    if (this->queue->results_queue_len < MAX_EVALUATION_QUEUE_LENGTH) {
        int i = this->queue->results_queue_len;
        copy(results, results + EVALUATION_METRICS, this->queue->results_queue[i]);
        this->queue->results_queue[i][EVALUATION_METRICS] = id;
        this->queue->results_queue_len++;
        submitted = true;
    }
    pthread_mutex_unlock(&(this->queue->results_lock));
    return submitted;
}

// Sets that the current process is the manager process
void EvaluationManager::setManager() {
    this->manager = true;
}

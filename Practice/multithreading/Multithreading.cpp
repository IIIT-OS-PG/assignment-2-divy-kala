#include <iostream>
#include <pthread.h>
#include <bits/stdc++.h>
using namespace std;

long long balance = 0;
pthread_mutex_t incbymutex = PTHREAD_MUTEX_INITIALIZER;
void * findsum(void * arg) {
    long long * tmp = (long long*) arg;
    long num = *tmp;
    long long sum = 0;
    for (int i = 0 ; i < num; i++) {
        sum+= i;
    }
    long long * ans = new long long (sum);
    pthread_exit(ans);
}

void * incrementby( void * off) {
    long long offset = *(long long *) off;
    for(int i = 0; i < 10000000; i++ ) {
        pthread_mutex_lock(&incbymutex);
        balance += offset;
        pthread_mutex_unlock(&incbymutex);

    }
    pthread_exit(NULL);

}

int main() {
    long long T;
    cin >> T;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_t t1;
	pthread_create(&t1,&attr,findsum, &T );

	pthread_t t2;
	pthread_attr_t attr2;
	pthread_attr_init(&attr2);

	pthread_create(&t2,&attr2,findsum, &T);

	long long * rest = NULL;
	long long * rest1 = NULL;
    pthread_join(t1, (void **) &rest);
    pthread_join(t2, (void **) &rest1);
    cout << "Sum is " << *rest<< endl << *rest1;
    delete rest;
    delete rest1;


    cout << endl;

    pthread_t t3, t4;
    long long off1,off2;
    off1= 1; off2 = -1;
    pthread_create(&t3,NULL,incrementby,&off1);
    pthread_create(&t4,NULL,incrementby,&off2);

    pthread_join(t3,NULL);
    pthread_join(t4,NULL);
    cout << "Balance is " << balance;



	return 0;
}

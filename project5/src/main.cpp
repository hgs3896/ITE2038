#include "bpt.h"
#include <iostream>
#include <fstream>
#include <functional>
// MAIN
/*
	#include <chrono>
	auto start = std::chrono::high_resolution_clock::now();
	auto end = std::chrono::high_resolution_clock::now();
	std::cout << "Analyzing Time : " << (end - start).count() << "ns" << std::endl;
*/

struct thr_arg
{
	int tid;
	record_key_t key;
};

constexpr auto N = 64;

void* func(void *arg){
	thr_arg* argument = static_cast<thr_arg*>(arg);
	std::fstream f(std::string("output_file") + std::to_string(argument->tid)
				   + "_" + std::to_string(argument->key) + ".txt", f.trunc | f.in | f.out);
	for ( int i = 0; i < 10; ++i )
	{
		auto result = find(argument->tid, i * N + argument->key);
		f << i * N + argument->key << " " << result[0] << " " << result[1] << std::endl;
		delete result;
	}
	f.close();
	return NULL;
}

int main( int argc, char *argv[] ) {
	init_db(128);
	open_table("test.db", 0);
	open_table("test2.db", 0);

	thread t[2 * N];
	thr_arg args[2 * N];

	for ( int i = 0; i < N; ++i )
	{
		args[i] = { 1, i };
		args[N + i] = { 2, i };
		pthread_create(&t[i], NULL, func, &args[i]);
		pthread_create(&t[N + i], NULL, func, &args[N + i]);
	}

	for ( int i = 0; i < 2 * N; ++i )
	{
		pthread_join(t[i], NULL);
	}

	shutdown_db();
	return EXIT_SUCCESS;
}

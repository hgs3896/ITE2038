#include "bpt.h"
#include <iostream>
// MAIN

int main( int argc, char ** argv ) {

	// Buffer Pool initialization
	init_db(128);

	int tid = 1;

	std::string query;

	auto num_col = getNumOfCols(tid);

	/*
#if TESTMODE
	usage();
#endif
	int input, range2;
	char instruction;
	record_val_t *values;
    do{
#if TESTMODE
		std::cout << "> ";
#endif
		values = nullptr;
		std::cin >> instruction;
        switch (instruction) {
        case 's':
			std::cin >> tid;
			std::cout << "Switch to the table " << tid << "\n";
			num_col = getNumOfCols(tid);
            break;
        case 'd':
			std::cin >> input;
            erase(tid, input);
            break;
        case 'i':
			std::cin >> input;

			values = new record_val_t[num_col - 1];
			for ( auto i = 0; i < num_col - 1; ++i )
				std::cin >> values[i];

            insert(tid, input, values);

			delete[] values;
            break;
        case 'f':
			std::cin >> input;

			if( values = find(tid, input) ){
				for ( auto i = 0; i < num_col - 1; ++i )
					std::cout << values[i] << ' ';
				std::cout << '\n';
				delete[] values;
			}
			else std::cout << "Not Exist\n";
            
            break;
        case 'r':
			std::cin >> input >> range2;
            find_and_print_range(tid, input, range2);
            break;
        case 'l':
            print_leaves(tid);
            break;
        case 'D':
			std::cin >> input >> range2;
            while(input <= range2){
                erase(tid, input++);
            }
            break;
        case 't':
            print_tree(tid);
            break;
		case 'j':
			std::cin >> query;
			std::cout << join(query.c_str()) << std::endl;
			break;
        default:
#if !TESTMODE
			usage();
#endif        
            break;
		case 'q':
			while ( std::cin.get() != (int)'\n' );
			return EXIT_SUCCESS;
        }
		while ( std::cin.get() != (int)'\n' );
	}
	while ( !std::cin.fail() );
	std::cout << "\n";
	*/
	auto addFileMode = true;
	auto success = 0;
	while ( std::getline(std::cin, query) )
	{
		if ( addFileMode )
		{
			if ( query == "R" )
			{
				addFileMode = false;
				continue;
			}
			auto str = const_cast<char*>(query.data());
			open_table(str, 2);
			continue;
		}		
		if ( query == "q" )
			break;
		auto result = join(query.c_str());
		std::getline(std::cin, query);
		if ( result == std::stoi(query) )
			++success;
	}
	std::cout << success << std::endl;
	return EXIT_SUCCESS;
}
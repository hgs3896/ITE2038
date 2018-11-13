#include "bpt.h"
// MAIN

int main( int argc, char ** argv ) {
#if !TESTMODE
    usage();
#endif
    int input, range2;
    char instruction, value[1024];

    int tid = 1;
    int table1 = open_table("test.db");
    int table2 = open_table("test2.db");

    // Buffer Pool initialization
    init_db(10);

#if !TESTMODE
    printf("> ");
#endif
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 's':
            scanf("%d", &input);
            tid = input;
            printf("Switch to the table %d\n", input);
            break;
        case 'd':
            scanf("%d", &input);
            delete(tid, input);
            // print_tree(root);
            break;
        case 'i':
            scanf("%d", &input);
            scanf("%s", value);
            insert(tid, input, value);
            break;
        case 'f':
            scanf("%d\b", &input);
            char *str = find(tid, input);
            if(str) printf("Key: %d, Value: %s\n", input, str);
            else    printf("Not Exist\n");
            free(str);
            break;
        case 'p':
            scanf("%d", &input);
            // find_and_print(root, input, instruction == 'p');
            break;
        case 'r':
            scanf("%d %d", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(tid, input, range2);
            break;
        case 'l':
            print_leaves(tid);
            break;
        case 'D':
            scanf("%d %d", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            while(input <= range2){
                delete(tid, input++);
            }
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(tid);
            break;
        case 'x':
            /*
            if (root)
                root = destroy_tree(root);
            */
            //print_tree(root);
            break;
        default:
        #if !TESTMODE
            usage();
        #endif
            break;
        }
        while (getchar() != (int)'\n');
        #if !TESTMODE
        printf("> ");
        #endif
    }
    printf("\n");

    return EXIT_SUCCESS;
}
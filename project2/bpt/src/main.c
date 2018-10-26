#include "bpt.h"
#define TESTMODE 0
// MAIN

int main( int argc, char ** argv ) {
#if !TESTMODE
    usage();
#endif
    offset_t root;
    int input, range2;
    char instruction, value[1024];

    open_db("test.db");
#if !TESTMODE
    printf("> ");
#endif
    while (scanf("%c", &instruction) != EOF) {
        root = getRootPageOffset(&header);
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            // root = delete(root, input);
            // print_tree(root);
            break;
        case 'i':
            scanf("%d", &input);
            scanf("%s", value);
            insert(input, value);
            break;
        case 'f':
            scanf("%d\b", &input);
            char *str = find(input);
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
            find_and_print_range(root, input, range2);
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
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
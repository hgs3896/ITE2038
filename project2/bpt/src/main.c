#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    usage();

    int input, range2;
    char instruction, value[1024];

    open_db("fuck_you.db");

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
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
            print_tree(getRootPageOffset(&header));
            break;
        case 'f':
            scanf("%d\b", &input);
            char * str = find(input);
            printf("%s\n", str ? str : "Not Found");
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
            // find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(getRootPageOffset(&header));
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(getRootPageOffset(&header));
            break;
        case 'x':
            /*
            if (root)
                root = destroy_tree(root);
            */
            //print_tree(root);
            break;
        default:
            usage();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
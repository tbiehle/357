#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct word_node {
    struct word_node *next, *prev;
    char word[1000];
} word_node;

typedef struct word_list {  // word_le: word list element
    struct word_node *head;
} word_list;

typedef struct alph_le {  // alph_le: alphabet list element
    struct alph_le *next, *prev;
    word_list *wlist;  // each letter has its own linked list of words
    char letter;
} alph_le;

alph_le * get_tail_of_list(alph_le *head) {
    // Returns the tail node of a doubly linked list.
    if (!head) { // list is empty
        return NULL;
    }

    alph_le * p = head;
    // search for tail
    while (p->next) {
        p = p->next;
    }

    return p;
}

alph_le *create_alphabet_list() {
    // Creates a doubly linked list of all the letters in the alphabet.
    char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
    alph_le *head = 0;

    for (int i = 0; i < 26; i ++) {
        alph_le *e = (alph_le*)malloc(sizeof(alph_le)); // create new node
        e->letter = alphabet[i]; // give new node its letter
        word_list *wlist = (word_list *)malloc(sizeof(word_list));
        e->wlist = wlist; // give new node its list

        if (i == 0) {  // list is empty, create head
            head = e;
        } else {  // list is not empty
            alph_le *tail = get_tail_of_list(head);  // find tail node
            tail->next = e;  // attach new node to tail
            e->prev = tail;
        }
    }

    return head;
}

void word_append(word_list *word_list, char word[]) {
    if (!word) return;
    if (!isalpha(word[0])) return;

    // appends a word to the end of a linked list.
    word_node *e = (word_node *)malloc(sizeof(word_node));  // create new word element for word
    strcpy(e->word, word); // set new word element's data to input word

    if (!word_list->head) { // word list is empty
        word_list->head = e; // set head of word list to new node
        int x = 0;
    } else { // word list is not empty
        word_node *curr_e = word_list->head; // entry point for list

        while (curr_e->next) {
            curr_e = curr_e->next;
        }
        curr_e->next = e; // attach new node to end of list
        e->prev = curr_e;
    }
}

void print_words(alph_le *alph_list) {
    // prints all words in the two-dimensional alphabet list.
    alph_le *curr_let = alph_list;

    while (curr_let) {
        word_list *curr_wlist = curr_let->wlist;
        if (curr_wlist) { // words starting with this letter are in the alphabet list
            word_node *curr_wn = curr_wlist->head;
            while (curr_wn) {
                printf("%s\n", curr_wn->word);
                curr_wn = curr_wn->next;
            }
        }

        curr_let = curr_let->next;
    }
}

void clear_lists(alph_le *alph_list) {
    alph_le *curr_let = alph_list;

    while (curr_let) {
        if (curr_let->wlist) {
            word_node *curr_wn = curr_let->wlist->head;
            while (curr_wn) { // free all nodes in word list
                word_node *next_wn = curr_wn->next;
                free(curr_wn);
                curr_wn = next_wn;
            }

            free(curr_let->wlist); // free the word list
        }
        alph_le *next_let = curr_let->next;
        free(curr_let);
        curr_let = next_let;
    }
}

int main() {
    alph_le *alphabet = create_alphabet_list();
    char user_in[100];

    while (strcmp(user_in, "print") != 0) {
        scanf("%s", user_in);
        if (strcmp(user_in, "print") == 0) {
            break;
        }
        char first_letter = tolower(user_in[0]);

        alph_le *curr_let = alphabet;
        while (curr_let) {
            if (curr_let->letter == first_letter) {
                word_append(curr_let->wlist, user_in);
            }
            curr_let = curr_let->next;
        }
    }

    print_words(alphabet);
    clear_lists(alphabet);

    return 0;
}
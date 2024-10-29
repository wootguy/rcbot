/*
 *  Copyright (C) 1998 Jason Hutchens
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the license or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the Gnu Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 *  Oct 2023 - Modified by wootguy
 */

#ifdef HLCOOP_BUILD
#include "hlcoop.h"
#else
#include "mmlib.h"
#endif

#include "megahal.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <chrono>
#include <thread>
#include <string>

using namespace std;

#define MIN(a,b) ((a)<(b))?(a):(b)

#define COOKIE "MegaHALv8"
#define TIMEOUT 1

ThreadSafeInt g_load_threads;
ThreadSafeInt g_save_threads;
ThreadSafeInt g_pending_brain_saves;

static bool file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return false;
    }
    fclose(file);
    return true;
}

void MegaHal::load_personality(const char* path)
{
    if (saveload_thread != NULL) {
        println("Waiting for model to finish loading/saving before loading personality again");
        saveload_thread->join();
        delete saveload_thread;
        saveload_thread = NULL;
    }

    if (model != NULL) {
        println("Personality already loaded.");
        return;
    }

    saveload_status.setValue(1);
    char* pathCopy = strdup(path);
    saveload_thread = new thread(&MegaHal::load_personality_thread, this, pathCopy);
}

void MegaHal::load_personality_thread(const char* path) {
    while (g_load_threads.getValue() > 0) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    g_load_threads.inc();

    brnpath = string(path) + ".brn";
    string trnpath = string(path) + ".trn";
    string banpath = string(path) + ".ban";
    string auxpath = string(path) + ".aux";
    string swppath = string(path) + ".swp";

    string rootDir = string(path);
    if (rootDir.find_last_of("/\\") != -1) {
        rootDir = rootDir.substr(0, rootDir.find_last_of("/\\") + 1);
    }

    if (!file_exists(trnpath.c_str())) {
        trnpath = rootDir + HAL_COMMON_TRN;
    }
    if (!file_exists(banpath.c_str())) {
        banpath = rootDir + HAL_COMMON_BAN;
    }
    if (!file_exists(auxpath.c_str())) {
        auxpath = rootDir + HAL_COMMON_AUX;
    }
    if (!file_exists(swppath.c_str())) {
        swppath = rootDir + HAL_COMMON_SWP;
    }

    free_everything();

    words = new_dictionary();

    model = new_model(order);

    // Read a dictionary containing banned keywords, auxiliary keywords, greeting keywords and swap keywords
    ban = initialize_list(banpath.c_str());
    aux = initialize_list(auxpath.c_str());
    swp = initialize_swap(swppath.c_str());

    if (load_model(brnpath.c_str(), model) == false) {
        train(trnpath.c_str());
        save_model_thread(false);
    }
    else {
        saveload_status.setValue(0);
    }

    g_load_threads.dec();
    println("Finished loading brain: %s", path);
    free((void*)path);
}

char* MegaHal::do_reply(char *input, bool learnFromInput)
{
    if (saveload_thread != NULL) {
        if (saveload_status.getValue() == 1) {
            println("Can't reply yet. Model is saving/loading");
            return "my brain is still loading";
        }
        saveload_thread->join();
        delete saveload_thread;
        saveload_thread = NULL;
    }

    if (model == NULL) {
        println("Can't reply. Model is null");
        return "My HAL model is null";
    }

    char* inputCopy = strdup(input);
    char *output = NULL;

    upper(inputCopy);

    make_words(inputCopy, words);

    output = generate_reply(model, words);
    capitalize(output);
    
    if (learnFromInput)
        learn(model, words);

    free(inputCopy);
    return output;
}

void MegaHal::learn_no_reply(char *input)
{
    if (saveload_thread != NULL) {
        if (saveload_status.getValue() == 1) {
            println("Can't learn yet. Model is saving/loading");
            return;
        }
        saveload_thread->join();
        delete saveload_thread;
        saveload_thread = NULL;
    }

    if (model == NULL) {
        println("Can't learn. Model is null");
    }

    upper(input);
    make_words(input, words);
    learn(model, words);
}

void MegaHal::save_model(bool deleteAfterSaving)
{
    if (saveload_thread != NULL) {
        println("Waiting for model to finish loading/saving before saving personality again");
        saveload_thread->join();
        delete saveload_thread;
        saveload_thread = NULL;
    }

    if (model == NULL) {
        println("Can't save model. Model was deleted.");
        return;
    }

    g_pending_brain_saves.inc();
    saveload_status.setValue(1);
    saveload_thread = new thread(&MegaHal::save_model_thread, this, deleteAfterSaving);
}

void MegaHal::save_model_thread(bool deleteAfterSave) {
    FILE* file;

    while (g_save_threads.getValue() > 0) {
        this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    g_save_threads.inc();

    if (brnpath.size() == 0) {
        printf("Can't save hal model. Not initialized.");
        goto cleanup;
    }

    file = fopen((brnpath + ".temp").c_str(), "wb");
    if (file == NULL) {
        warn("save_model", "Unable to open file '%s'", brnpath.c_str());
        goto cleanup;
    }

    fwrite(COOKIE, sizeof(char), strlen(COOKIE), file);
    fwrite(&(model->order), sizeof(uint8_t), 1, file);
    save_tree(file, model->forward);
    save_tree(file, model->backward);
    save_dictionary(file, model->dictionary);

    fclose(file);
    remove(brnpath.c_str());
    rename((brnpath + ".temp").c_str(), brnpath.c_str());

    if (deleteAfterSave) {
        free_everything();
    }

    println("Finished saving brain: %s", brnpath.c_str());

cleanup:
    saveload_status.setValue(0);
    g_save_threads.dec();
    g_pending_brain_saves.dec();
}

MegaHal::~MegaHal() {
    free_everything();
}

//
// Private functions
//

void MegaHal::free_everything() {
    free_model(model);
    free_words(ban);
    free_dictionary(ban);
    free(ban);
    free_words(aux);
    free_dictionary(aux);
    free(aux);
    free_swap(swp);

    //free_words(words); // word data points to memory handled by calling funcs
    free_dictionary(words);
    free(words);

    model = NULL;
    ban = NULL;
    aux = NULL;
    swp = NULL;
    words = NULL;
}

// Print the specified message to the error file.
void MegaHal::error(char *title, char *fmt, ...)
{
    va_list argptr;
    static char string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    fprintf(stderr, "MegaHAL died. %s: %s \n", title, string);

    free_everything();
}

bool MegaHal::warn(char *title, char *fmt, ...)
{
    va_list argptr;
    static char string[1024];

    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    fprintf(stderr, "MegaHAL emitted a warning. %s: %s\n", title, string);

    return(true);
}

// Convert a string to look nice.
void MegaHal::capitalize(char *string)
{
    register unsigned int i;
    bool start=true;

    for(i=0; i<strlen(string); ++i) {
	if(isalpha((unsigned char)string[i])) {
	    if(start==true) string[i]=(char)toupper((unsigned char)string[i]);
	    else string[i]=(char)tolower((unsigned char)string[i]);
	    start=false;
	}
	if((i>2)&&(strchr("!.?", string[i-1])!=NULL)&&(isspace((unsigned char)string[i])))
	    start=true;
    }
}

// Convert a string to its uppercase representation.
void MegaHal::upper(char *string)
{
    register unsigned int i;

    for(i=0; i<strlen(string); ++i)
        string[i] = (char)toupper((unsigned char)string[i]);
}

// Add a word to a dictionary, and return the identifierassigned to the word.If the word already
// exists in the dictionary, then return its current identifier without adding it again.
uint16_t MegaHal::add_word(HAL_DICTIONARY *dictionary, HAL_STRING word)
{
    register int i;
    int position;
    bool found;

    /*
     *		If the word's already in the dictionary, there is no need to add it
     */
    position=search_dictionary(dictionary, word, &found);
    if(found==true) goto succeed;

    /*
     *		Increase the number of words in the dictionary
     */
    dictionary->size+=1;

    /*
     *		Allocate one more entry for the word index
     */
    if(dictionary->index==NULL) {
	dictionary->index=(uint16_t *)malloc(sizeof(uint16_t)*
					  (dictionary->size));
    } else {
	dictionary->index=(uint16_t *)realloc((uint16_t *)
					   (dictionary->index),sizeof(uint16_t)*(dictionary->size));
    }
    if(dictionary->index==NULL) {
	error("add_word", "Unable to reallocate the index.");
	goto fail;
    }

    /*
     *		Allocate one more entry for the word array
     */
    if(dictionary->entry==NULL) {
	dictionary->entry=(HAL_STRING *)malloc(sizeof(HAL_STRING)*(dictionary->size));
    } else {
	dictionary->entry=(HAL_STRING *)realloc((HAL_STRING *)(dictionary->entry),
					    sizeof(HAL_STRING)*(dictionary->size));
    }
    if(dictionary->entry==NULL) {
	error("add_word", "Unable to reallocate the dictionary to %d elements.", dictionary->size);
	goto fail;
    }

    /*
     *		Copy the new word into the word array
     */
    dictionary->entry[dictionary->size-1].length=word.length;
    dictionary->entry[dictionary->size-1].word=(char *)malloc(sizeof(char)*
							      (word.length));
    if(dictionary->entry[dictionary->size-1].word==NULL) {
	error("add_word", "Unable to allocate the word.");
	goto fail;
    }
    for(i=0; i<word.length; ++i)
	dictionary->entry[dictionary->size-1].word[i]=word.word[i];

    /*
     *		Shuffle the word index to keep it sorted alphabetically
     */
    for(i=(dictionary->size-1); i>position; --i)
	dictionary->index[i]=dictionary->index[i-1];

    /*
     *		Copy the new symbol identifier into the word index
     */
    dictionary->index[position]=dictionary->size-1;

succeed:
    return(dictionary->index[position]);

fail:
    return(0);
}

// Search the dictionary for the specified word, returning its position in the index if found, 
// or the position where it should be inserted otherwise.
int MegaHal::search_dictionary(HAL_DICTIONARY *dictionary, HAL_STRING word, bool *find)
{
    int position;
    int min;
    int max;
    int middle;
    int compar;

    /*
     *		If the dictionary is empty, then obviously the word won't be found
     */
    if(dictionary->size==0) {
	position=0;
	goto notfound;
    }

    /*
     *		Initialize the lower and upper bounds of the search
     */
    min=0;
    max=dictionary->size-1;
    /*
     *		Search repeatedly, halving the search space each time, until either
     *		the entry is found, or the search space becomes empty
     */
    while(true) {
	/*
	 *		See whether the middle element of the search space is greater
	 *		than, equal to, or less than the element being searched for.
	 */
	middle=(min+max)/2;
	compar=wordcmp(word, dictionary->entry[dictionary->index[middle]]);
	/*
	 *		If it is equal then we have found the element.  Otherwise we
	 *		can halve the search space accordingly.
	 */
	if(compar==0) {
	    position=middle;
	    goto found;
	} else if(compar>0) {
	    if(max==middle) {
		position=middle+1;
		goto notfound;
	    }
	    min=middle+1;
	} else {
	    if(min==middle) {
		position=middle;
		goto notfound;
	    }
	    max=middle-1;
	}
    }

found:
    *find=true;
    return(position);

notfound:
    *find=false;
    return(position);
}

// Return the symbol corresponding to the word specified. We assume that the word with index zero
// is equal to a NULL word, indicating an error condition.
uint16_t MegaHal::find_word(HAL_DICTIONARY *dictionary, HAL_STRING word)
{
    int position;
    bool found;

    position=search_dictionary(dictionary, word, &found);

    if(found==true) return(dictionary->index[position]);
    else return(0);
}

// Compare two words, and return an integer indicating whether the first word is less than,
// equal to or greater than the second word
int MegaHal::wordcmp(HAL_STRING word1, HAL_STRING word2)
{
    register int i;
    int bound;

    bound=MIN(word1.length,word2.length);

    for(i=0; i<bound; ++i)
	if(toupper((unsigned char)word1.word[i])!=toupper((unsigned char)word2.word[i]))
	    return((int)(toupper((unsigned char)word1.word[i])-toupper((unsigned char)word2.word[i])));

    if(word1.length<word2.length) return(-1);
    if(word1.length>word2.length) return(1);

    return(0);
}

// Release the memory consumed by the dictionary.
void MegaHal::free_dictionary(HAL_DICTIONARY *dictionary)
{
    if(dictionary==NULL)
        return;
    if(dictionary->entry!=NULL) {
	    free(dictionary->entry);
	    dictionary->entry=NULL;
    }
    if(dictionary->index!=NULL) {
	    free(dictionary->index);
	    dictionary->index=NULL;
    }
    dictionary->size=0;
}

void MegaHal::free_model(HAL_MODEL *model)
{
    if(model==NULL) return;
    if(model->forward!=NULL) {
	    free_tree(model->forward);
    }
    if(model->backward!=NULL) {
	    free_tree(model->backward);
    }
    if(model->context!=NULL) {
	    free(model->context);
    }
    if(model->dictionary!=NULL) {
	    free_words(model->dictionary);
	    free_dictionary(model->dictionary);
        free(model->dictionary);
    }
    free(model);
}

void MegaHal::free_tree(HAL_TREE *tree)
{
    register unsigned int i;

    if(tree==NULL) return;

    if(tree->tree!=NULL) {
	for(i=0; i<tree->branch; ++i) {
	    free_tree(tree->tree[i]);
	}
	free(tree->tree);
    }
    free(tree);
}

// Add dummy words to the dictionary.
void MegaHal::initialize_dictionary(HAL_DICTIONARY *dictionary)
{
    HAL_STRING word={ 7, "<ERROR>" };
    HAL_STRING end={ 5, "<FIN>" };

    (void)add_word(dictionary, word);
    (void)add_word(dictionary, end);
}

// Allocate room for a new dictionary.
HAL_DICTIONARY* MegaHal::new_dictionary(void)
{
    HAL_DICTIONARY *dictionary=NULL;

    dictionary=(HAL_DICTIONARY *)malloc(sizeof(HAL_DICTIONARY));
    if(dictionary==NULL) {
	    error("new_dictionary", "Unable to allocate dictionary.");
	    return(NULL);
    }

    dictionary->size=0;
    dictionary->index=NULL;
    dictionary->entry=NULL;

    return(dictionary);
}

// Save a dictionary to the specified file.
void MegaHal::save_dictionary(FILE *file, HAL_DICTIONARY *dictionary)
{
    register unsigned int i;

    fwrite(&(dictionary->size), sizeof(uint32_t), 1, file);
    printf("Saving dictionary\n");
    for(i=0; i<dictionary->size; ++i) {
	save_word(file, dictionary->entry[i]);
    }
}

// Load a dictionary from the specified file.
void MegaHal::load_dictionary(FILE *file, HAL_DICTIONARY *dictionary)
{
    register int i;
    int size;

    fread(&size, sizeof(uint32_t), 1, file);
    for(i=0; i<size; ++i) {
	load_word(file, dictionary);
    }
}

// Save a dictionary word to a file.
void MegaHal::save_word(FILE *file, HAL_STRING word)
{
    register unsigned int i;

    fwrite(&(word.length), sizeof(uint8_t), 1, file);
    for(i=0; i<word.length; ++i)
	fwrite(&(word.word[i]), sizeof(char), 1, file);
}

// Load a dictionary word from a file.
void MegaHal::load_word(FILE *file, HAL_DICTIONARY *dictionary)
{
    register unsigned int i;
    HAL_STRING word;

    fread(&(word.length), sizeof(uint8_t), 1, file);
    word.word=(char *)malloc(sizeof(char)*word.length);
    if(word.word==NULL) {
	error("load_word", "Unable to allocate word");
	return;
    }
    for(i=0; i<word.length; ++i)
	fread(&(word.word[i]), sizeof(char), 1, file);
    add_word(dictionary, word);
    free(word.word);
}

// Allocate a new node for the n-gram tree, and initialise its contents to sensible values.
HAL_TREE* MegaHal::new_node(void)
{
    HAL_TREE *node=NULL;

    /*
     *		Allocate memory for the new node
     */
    node=(HAL_TREE *)malloc(sizeof(HAL_TREE));
    if(node==NULL) {
	error("new_node", "Unable to allocate the node.");
	goto fail;
    }

    /*
     *		Initialise the contents of the node
     */
    node->symbol=0;
    node->usage=0;
    node->count=0;
    node->branch=0;
    node->tree=NULL;

    return(node);

fail:
    if(node!=NULL) free(node);
    return(NULL);
}

// Create and initialise a new ngram model.
HAL_MODEL* MegaHal::new_model(int order)
{
    HAL_MODEL *model=NULL;

    model=(HAL_MODEL *)malloc(sizeof(HAL_MODEL));
    if(model==NULL) {
	    error("new_model", "Unable to allocate model.");
	    goto fail;
    }
    memset(model, 0, sizeof(HAL_MODEL));

    model->order=order;
    model->forward=new_node();
    model->backward=new_node();
    model->context=(HAL_TREE **)malloc(sizeof(HAL_TREE *)*(order+2));
    if(model->context==NULL) {
	    error("new_model", "Unable to allocate context array.");
	    goto fail;
    }
    initialize_context(model);
    model->dictionary=new_dictionary();
    initialize_dictionary(model->dictionary);

    return(model);

fail:
    return(NULL);
}

// Update the model with the specified symbol.
void MegaHal::update_model(HAL_MODEL *model, int symbol)
{
    register unsigned int i;

    /*
     *		Update all of the models in the current context with the specified
     *		symbol.
     */
    for(i=(model->order+1); i>0; --i)
	if(model->context[i-1]!=NULL)
	    model->context[i]=add_symbol(model->context[i-1], (uint16_t)symbol);

    return;
}

// Update the context of the model without adding the symbol.
void MegaHal::update_context(HAL_MODEL *model, int symbol)
{
    register unsigned int i;

    for(i=(model->order+1); i>0; --i)
	if(model->context[i-1]!=NULL)
	    model->context[i]=find_symbol(model->context[i-1], symbol);
}

// Update the statistics of the specified tree with the specified symbol, which may mean
// growing the tree if the symbol hasn't been seen in this context before.
HAL_TREE* MegaHal::add_symbol(HAL_TREE *tree, uint16_t symbol)
{
    HAL_TREE *node=NULL;

    /*
     *		Search for the symbol in the subtree of the tree node.
     */
    node=find_symbol_add(tree, symbol);

    /*
     *		Increment the symbol counts
     */
    if((node->count<65535)) {
	node->count+=1;
	tree->usage+=1;
    }

    return(node);
}

// Return a pointer to the child node, if one exists, which contains the specified symbol.
HAL_TREE* MegaHal::find_symbol(HAL_TREE *node, int symbol)
{
    register unsigned int i;
    HAL_TREE *found=NULL;
    bool found_symbol=false;

    /*
     *		Perform a binary search for the symbol.
     */
    i=search_node(node, symbol, &found_symbol);
    if(found_symbol==true) found=node->tree[i];

    return(found);
}

// This function is conceptually similar to find_symbol, apart from the fact that if th
// symbol is not found, a new node is automatically allocatedand added to the tree.
HAL_TREE* MegaHal::find_symbol_add(HAL_TREE *node, int symbol)
{
    register unsigned int i;
    HAL_TREE *found=NULL;
    bool found_symbol=false;

    /*
     *		Perform a binary search for the symbol.  If the symbol isn't found,
     *		attach a new sub-node to the tree node so that it remains sorted.
     */
    i=search_node(node, symbol, &found_symbol);
    if(found_symbol==true) {
	found=node->tree[i];
    } else {
	found=new_node();
	found->symbol=symbol;
	add_node(node, found, i);
    }

    return(found);
}

// Attach a new child node to the sub-tree of the tree specified.
void MegaHal::add_node(HAL_TREE *tree, HAL_TREE *node, int position)
{
    register int i;

    /*
     *		Allocate room for one more child node, which may mean allocating
     *		the sub-tree from scratch.
     */
    if(tree->tree==NULL) {
	tree->tree=(HAL_TREE **)malloc(sizeof(HAL_TREE *)*(tree->branch+1));
    } else {
	tree->tree=(HAL_TREE **)realloc((HAL_TREE **)(tree->tree),sizeof(HAL_TREE *)*
				    (tree->branch+1));
    }
    if(tree->tree==NULL) {
	error("add_node", "Unable to reallocate subtree.");
	return;
    }

    /*
     *		Shuffle the nodes down so that we can insert the new node at the
     *		subtree index given by position.
     */
    for(i=tree->branch; i>position; --i)
	tree->tree[i]=tree->tree[i-1];

    /*
     *		Add the new node to the sub-tree.
     */
    tree->tree[position]=node;
    tree->branch+=1;
}

// Perform a binary search for the specified symbol on the subtree of the given node.
// Return the position of the child node in the subtree if the symbol was found, or the
// position where it should be inserted to keep the subtree sorted if it wasn't.
int MegaHal::search_node(HAL_TREE *node, int symbol, bool *found_symbol)
{
    register unsigned int position;
    int min;
    int max;
    int middle;
    int compar;

    /*
     *		Handle the special case where the subtree is empty.
     */
    if(node->branch==0) {
	position=0;
	goto notfound;
    }

    /*
     *		Perform a binary search on the subtree.
     */
    min=0;
    max=node->branch-1;
    while(true) {
	middle=(min+max)/2;
	compar=symbol-node->tree[middle]->symbol;
	if(compar==0) {
	    position=middle;
	    goto found;
	} else if(compar>0) {
	    if(max==middle) {
		position=middle+1;
		goto notfound;
	    }
	    min=middle+1;
	} else {
	    if(min==middle) {
		position=middle;
		goto notfound;
	    }
	    max=middle-1;
	}
    }

found:
    *found_symbol=true;
    return(position);

notfound:
    *found_symbol=false;
    return(position);
}

// Set the context of the model to a default value.
void MegaHal::initialize_context(HAL_MODEL *model)
{
    register unsigned int i;

    for(i=0; i<=model->order; ++i) model->context[i]=NULL;
}

// Learn from the user's input.
void MegaHal::learn(HAL_MODEL *model, HAL_DICTIONARY *words)
{
    register unsigned int i;
    register int j;
    uint16_t symbol;

    /*
     *		We only learn from inputs which are long enough
     */
    if(words->size<=(model->order)) return;

    /*
     *		Train the model in the forwards direction.  Start by initializing
     *		the context of the model.
     */
    initialize_context(model);
    model->context[0]=model->forward;
    for(i=0; i<words->size; ++i) {
	/*
	 *		Add the symbol to the model's dictionary if necessary, and then
	 *		update the forward model accordingly.
	 */
	symbol=add_word(model->dictionary, words->entry[i]);
	update_model(model, symbol);
    }
    /*
     *		Add the sentence-terminating symbol.
     */
    update_model(model, 1);

    /*
     *		Train the model in the backwards direction.  Start by initializing
     *		the context of the model.
     */
    initialize_context(model);
    model->context[0]=model->backward;
    for(j=words->size-1; j>=0; --j) {
	/*
	 *		Find the symbol in the model's dictionary, and then update
	 *		the backward model accordingly.
	 */
	symbol=find_word(model->dictionary, words->entry[j]);
	update_model(model, symbol);
    }
    /*
     *		Add the sentence-terminating symbol.
     */
    update_model(model, 1);

    return;
}

// Infer a MegaHAL brain from the contents of a text file.
void MegaHal::train(const char *filename)
{
    FILE *file;
    char buffer[1024];
    HAL_DICTIONARY *words=NULL;
    int length;

    if(filename==NULL)
        return;

    file=fopen(filename, "r");
    if(file==NULL) {
	    printf("Unable to find the personality %s\n", filename);
	    return;
    }

    fseek(file, 0, 2);
    length=ftell(file);
    rewind(file);

    words=new_dictionary();

    printf("Training from file\n");
    while(!feof(file)) {
	    if(fgets(buffer, 1024, file)==NULL)
            break;

	    if(buffer[0]=='#')
            continue;

	    buffer[strlen(buffer)-1]='\0';

	    upper(buffer);
	    make_words(buffer, words);
	    learn(model, words);
    }

    free_dictionary(words);
    fclose(file);
}

// Display the dictionary for training purposes.
void MegaHal::write_dictionary()
{
    register unsigned int i;
    register unsigned int j;
    FILE *file;
    HAL_DICTIONARY* dictionary = model->dictionary;

    file=fopen("megahal.dic", "w");
    if(file==NULL) {
	    return;
    }

    for(i=0; i<dictionary->size; ++i) {
	for(j=0; j<dictionary->entry[i].length; ++j)
	    fprintf(file, "%c", dictionary->entry[i].word[j]);
	fprintf(file, "\n");
    }

    fclose(file);
}

// Save a tree structure to the specified file.
void MegaHal::save_tree(FILE *file, HAL_TREE *node)
{
    static int level=0;
    register unsigned int i;

    fwrite(&(node->symbol), sizeof(uint16_t), 1, file);
    fwrite(&(node->usage), sizeof(uint32_t), 1, file);
    fwrite(&(node->count), sizeof(uint16_t), 1, file);
    fwrite(&(node->branch), sizeof(uint16_t), 1, file);

    for(i=0; i<node->branch; ++i) {
	++level;
	save_tree(file, node->tree[i]);
	--level;
    }
}

// Load a tree structure from the specified file.
void MegaHal::load_tree(FILE *file, HAL_TREE *node)
{
    register unsigned int i;

    fread(&(node->symbol), sizeof(uint16_t), 1, file);
    fread(&(node->usage), sizeof(uint32_t), 1, file);
    fread(&(node->count), sizeof(uint16_t), 1, file);
    fread(&(node->branch), sizeof(uint16_t), 1, file);

    if(node->branch==0) return;

    node->tree=(HAL_TREE **)malloc(sizeof(HAL_TREE *)*(node->branch));
    if(node->tree==NULL) {
	error("load_tree", "Unable to allocate subtree");
	return;
    }

    for(i=0; i<node->branch; ++i) {
	node->tree[i]=new_node();
	load_tree(file, node->tree[i]);
    }
}

// Load a model into memory.
bool MegaHal::load_model(const char *filename, HAL_MODEL *model)
{
    FILE *file;
    char cookie[16];

    if(filename==NULL) return(false);

    file=fopen(filename, "rb");

    if(file==NULL) {
	    warn("load_model", "Unable to open file `%s'", filename);
	    return(false);
    }

    fread(cookie, sizeof(char), strlen(COOKIE), file);
    if(strncmp(cookie, COOKIE, strlen(COOKIE)) != 0) {
	    warn("load_model", "File `%s' is not a MegaHAL brain", filename);
        fclose(file);
        return false;
    }

    fread(&(model->order), sizeof(uint8_t), 1, file);
    load_tree(file, model->forward);
    load_tree(file, model->backward);
    load_dictionary(file, model->dictionary);
    fclose(file);

    return(true);
}

// Break a string into an array of words.
void MegaHal::make_words(char *input, HAL_DICTIONARY *words)
{
    int offset=0;

    /*
     *		Clear the entries in the dictionary
     */
    free_dictionary(words);

    /*
     *		If the string is empty then do nothing, for it contains no words.
     */
    if(strlen(input)==0) return;

    

    /*
     *		Loop forever.
     */
    while(1) {

	/*
	 *		If the current character is of the same type as the previous
	 *		character, then include it in the word.  Otherwise, terminate
	 *		the current word.
	 */

	if(boundary(input, offset)) {
	    /*
	     *		Add the word to the dictionary
	     */
	    if(words->entry==NULL)
		words->entry=(HAL_STRING *)malloc((words->size+1)*sizeof(HAL_STRING));
	    else
		words->entry=(HAL_STRING *)realloc(words->entry, (words->size+1)*sizeof(HAL_STRING));

	    if(words->entry==NULL) {
		error("make_words", "Unable to reallocate dictionary");
		return;
	    }

	    words->entry[words->size].length=offset;
	    words->entry[words->size].word=input;
	    words->size+=1;

	    if(offset==(int)strlen(input)) break;
	    input+=offset;
	    offset=0;
	} else {
	    ++offset;
	}
    }

    return;
}

// Return whether or not a word boundary exists in a string at the specified location.
bool MegaHal::boundary(char *string, int position)
{
    if(position==0)
	    return(false);

    if(position==(int)strlen(string))
	    return(true);

    if(isWordSep(string[position]) && isWordChar(string[position-1]) && isWordChar(string[position+1]))
	    return(false);

    if(position>1 && isWordSep(string[position-1]) && isWordChar(string[position-2]) && isWordChar(string[position]))
	    return(false);

    if (isWordChar(string[position]) != isWordChar(string[position - 1]))
	    return(true);

    return(false);
}

// Take a string of user input and return a string of output which may vaguely be construed
// as containing a reply to whatever is in the input string.
char* MegaHal::generate_reply(HAL_MODEL *model, HAL_DICTIONARY *words)
{
    HAL_DICTIONARY *replywords;
    HAL_DICTIONARY *keywords;
    float surprise;
    float max_surprise;

    /*
     *		Create an array of keywords from the words in the user's input
     */
    keywords=make_keywords(model, words);

    /*
     *		Make sure some sort of reply exists
     */
    snprintf(replyBuffer, HAL_MAX_REPLY_LEN, "I don't know enough to answer you yet!");
    replyBuffer[HAL_MAX_REPLY_LEN-1] = '\0';

    // default random message, for when relevent replies all match the input message exactly
    HAL_DICTIONARY* dummy = new_dictionary();
    replywords = reply(model, dummy);
    if (dissimilar(words, replywords) == true) make_output(replywords);

    /*
     *		Loop for the specified waiting period, generating and evaluating
     *		replies
     */
    max_surprise = -1.0f;
    
    for (int i = 0; i < reply_iterations; i++) {
	    replywords=reply(model, keywords);
	    surprise=evaluate_reply(model, keywords, replywords);

        int length = 1;
        for (int k = 0; k < (int)replywords->size; ++k)
            length += replywords->entry[k].length;

        if (length >= HAL_MAX_REPLY_LEN) {
            println("[MegaHal] max reply length exceeded %d > %d)\n", length, HAL_MAX_REPLY_LEN);
            continue;
        }

	    if ((surprise>max_surprise) && (dissimilar(words, replywords)==true)) {
	        max_surprise=surprise;
	        make_output(replywords);
            //printf("%d / %d - %s\n", i, reply_iterations, replyBuffer);
	    }
    }

    /*
     *		Return the best answer we generated
     */
    return replyBuffer;
}

// Return true or false depending on whether the dictionaries are the same or not.
bool MegaHal::dissimilar(HAL_DICTIONARY *words1, HAL_DICTIONARY *words2)
{
    register unsigned int i;

    if(words1->size!=words2->size) return(true);
    for(i=0; i<words1->size; ++i)
	if(wordcmp(words1->entry[i], words2->entry[i])!=0) return(true);
    return(false);
}

// Put all the interesting words from the user's input into a keywords dictionary,
// which will be used when generating a reply.
HAL_DICTIONARY* MegaHal::make_keywords(HAL_MODEL *model, HAL_DICTIONARY *words)
{
    static HAL_DICTIONARY *keys=NULL;
    register unsigned int i;
    register unsigned int j;
    int c;

    if(keys==NULL) keys=new_dictionary();
    for(i=0; i<keys->size; ++i) free(keys->entry[i].word);
    free_dictionary(keys);

    for(i=0; i<words->size; ++i) {
	/*
	 *		Find the symbol ID of the word.  If it doesn't exist in
	 *		the model, or if it begins with a non-alphanumeric
	 *		character, or if it is in the exclusion array, then
	 *		skip over it.
	 */
	c=0;
	for(j=0; j<swp->size; ++j)
	    if(wordcmp(swp->from[j], words->entry[i])==0) {
		add_key(model, keys, swp->to[j]);
		++c;
	    }
	if(c==0) add_key(model, keys, words->entry[i]);
    }

    if(keys->size>0) for(i=0; i<words->size; ++i) {

	c=0;
	for(j=0; j<swp->size; ++j)
	    if(wordcmp(swp->from[j], words->entry[i])==0) {
		add_aux(model, keys, swp->to[j]);
		++c;
	    }
	if(c==0) add_aux(model, keys, words->entry[i]);
    }

    return(keys);
}

// Add a word to the keyword dictionary.
void MegaHal::add_key(HAL_MODEL *model, HAL_DICTIONARY *keys, HAL_STRING word)
{
    int symbol;

    symbol=find_word(model->dictionary, word);
    if(symbol==0) return;
    if(isalnum((unsigned char)word.word[0])==0) return;
    symbol=find_word(ban, word);
    if(symbol!=0) return;
    symbol=find_word(aux, word);
    if(symbol!=0) return;

    add_word(keys, word);
}

// Add an auxilliary keyword to the keyword dictionary.
void MegaHal::add_aux(HAL_MODEL *model, HAL_DICTIONARY *keys, HAL_STRING word)
{
    int symbol;

    symbol=find_word(model->dictionary, word);
    if(symbol==0) return;
    if(isalnum((unsigned char)word.word[0])==0) return;
    symbol=find_word(aux, word);
    if(symbol==0) return;

    add_word(keys, word);
}

// Generate a dictionary of reply words appropriate to the given dictionary of keywords.
HAL_DICTIONARY* MegaHal::reply(HAL_MODEL *model, HAL_DICTIONARY *keys)
{
    static HAL_DICTIONARY *replies=NULL;
    register int i;
    int symbol;
    bool start=true;

    if(replies==NULL) replies=new_dictionary();
    free_dictionary(replies);

    /*
     *		Start off by making sure that the model's context is empty.
     */
    initialize_context(model);
    model->context[0]=model->forward;
    used_key=false;

    /*
     *		Generate the reply in the forward direction.
     */
    while(true) {
	/*
	 *		Get a random symbol from the current context.
	 */
	if(start==true) symbol=seed(model, keys);
	else symbol=babble(model, keys, replies);
	if((symbol==0)||(symbol==1)) break;
	start=false;

	/*
	 *		Append the symbol to the reply dictionary.
	 */
	if(replies->entry==NULL)
	    replies->entry=(HAL_STRING *)malloc((replies->size+1)*sizeof(HAL_STRING));
	else
	    replies->entry=(HAL_STRING *)realloc(replies->entry, (replies->size+1)*sizeof(HAL_STRING));
	if(replies->entry==NULL) {
	    error("reply", "Unable to reallocate dictionary");
	    return(NULL);
	}

	replies->entry[replies->size].length=
	    model->dictionary->entry[symbol].length;
	replies->entry[replies->size].word=
	    model->dictionary->entry[symbol].word;
	replies->size+=1;

	/*
	 *		Extend the current context of the model with the current symbol.
	 */
	update_context(model, symbol);
    }

    /*
     *		Start off by making sure that the model's context is empty.
     */
    initialize_context(model);
    model->context[0]=model->backward;

    /*
     *		Re-create the context of the model from the current reply
     *		dictionary so that we can generate backwards to reach the
     *		beginning of the string.
     */
    if(replies->size>0) for(i=MIN(replies->size-1, model->order); i>=0; --i) {
	symbol=find_word(model->dictionary, replies->entry[i]);
	update_context(model, symbol);
    }

    /*
     *		Generate the reply in the backward direction.
     */
    while(true) {
	/*
	 *		Get a random symbol from the current context.
	 */
	symbol=babble(model, keys, replies);
	if((symbol==0)||(symbol==1)) break;

	/*
	 *		Prepend the symbol to the reply dictionary.
	 */
	if(replies->entry==NULL)
	    replies->entry=(HAL_STRING *)malloc((replies->size+1)*sizeof(HAL_STRING));
	else
	    replies->entry=(HAL_STRING *)realloc(replies->entry, (replies->size+1)*sizeof(HAL_STRING));
	if(replies->entry==NULL) {
	    error("reply", "Unable to reallocate dictionary");
	    return(NULL);
	}

	/*
	 *		Shuffle everything up for the prepend.
	 */
	for(i=replies->size; i>0; --i) {
	    replies->entry[i].length=replies->entry[i-1].length;
	    replies->entry[i].word=replies->entry[i-1].word;
	}

	replies->entry[0].length=model->dictionary->entry[symbol].length;
	replies->entry[0].word=model->dictionary->entry[symbol].word;
	replies->size+=1;

	/*
	 *		Extend the current context of the model with the current symbol.
	 */
	update_context(model, symbol);
    }

    return(replies);
}

// Measure the average surprise of keywords relative to the language model.
float MegaHal::evaluate_reply(HAL_MODEL *model, HAL_DICTIONARY *keys, HAL_DICTIONARY *words)
{
    register unsigned int i;
    register int j;
    register int k;
    int symbol;
    float probability;
    int count;
    float entropy=(float)0.0;
    HAL_TREE *node;
    int num=0;

    if(words->size<=0) return((float)0.0);
    initialize_context(model);
    model->context[0]=model->forward;
    for(i=0; i<words->size; ++i) {
	symbol=find_word(model->dictionary, words->entry[i]);

	if(find_word(keys, words->entry[i])!=0) {
	    probability=(float)0.0;
	    count=0;
	    ++num;
	    for(j=0; j<model->order; ++j) if(model->context[j]!=NULL) {

		node=find_symbol(model->context[j], symbol);
		probability+=(float)(node->count)/
		    (float)(model->context[j]->usage);
		++count;

	    }

	    if(count>0.0) entropy-=(float)log(probability/(float)count);
	}

	update_context(model, symbol);
    }

    initialize_context(model);
    model->context[0]=model->backward;
    for(k=words->size-1; k>=0; --k) {
	symbol=find_word(model->dictionary, words->entry[k]);

	if(find_word(keys, words->entry[k])!=0) {
	    probability=(float)0.0;
	    count=0;
	    ++num;
	    for(j=0; j<model->order; ++j) if(model->context[j]!=NULL) {

		node=find_symbol(model->context[j], symbol);
		probability+=(float)(node->count)/
		    (float)(model->context[j]->usage);
		++count;

	    }

	    if(count>0.0) entropy-=(float)log(probability/(float)count);
	}

	update_context(model, symbol);
    }

    if(num>=8) entropy/=(float)sqrt(num-1);
    if(num>=16) entropy/=(float)num;

    return(entropy);
}

// Generate a string from the dictionary of reply words.
char* MegaHal::make_output(HAL_DICTIONARY *words)
{
    static char *output=NULL;
    register unsigned int i;
    register int j;
    int length;
    static char *output_none=NULL;

    if(words->size == 0) {
        snprintf(replyBuffer, HAL_MAX_REPLY_LEN, "I am utterly speechless!");
        replyBuffer[HAL_MAX_REPLY_LEN-1] = '\0';
        return replyBuffer;
    }

    length = 1;
    for(i = 0; i < words->size; ++i)
        length += words->entry[i].length;

    if (length >= HAL_MAX_REPLY_LEN) {
        snprintf(replyBuffer, HAL_MAX_REPLY_LEN, "HAL_MAX_REPLY_LEN exceeded!");
        replyBuffer[HAL_MAX_REPLY_LEN-1] = '\0';
        return replyBuffer;
    }

    length = 0;
    for (i = 0; i < words->size; ++i) {
        for (j = 0; j < words->entry[i].length; ++j) {
            replyBuffer[length++] = words->entry[i].word[j];
        }
    }

    replyBuffer[length] = '\0';

    return replyBuffer;
}

// Return a random symbol from the current context, or a zero symbol identifier if we've 
// reached either the start or end of the sentence.Select the symbol based on probabilities,
// favouring keywords. In all cases, use the longest available context to choose the symbol.
int MegaHal::babble(HAL_MODEL *model, HAL_DICTIONARY *keys, HAL_DICTIONARY *words)
{
    HAL_TREE *node;
    register int i;
    int count;
    int symbol = 0;

    node = NULL;

    /*
     *		Select the longest available context.
     */
    for(i=0; i<=model->order; ++i)
	if(model->context[i]!=NULL)
	    node=model->context[i];

    if(node->branch==0) return(0);

    /*
     *		Choose a symbol at random from this context.
     */
    i=rnd(node->branch);
    count=rnd(node->usage);
    while(count>=0) {
	/*
	 *		If the symbol occurs as a keyword, then use it.  Only use an
	 *		auxilliary keyword if a normal keyword has already been used.
	 */
	symbol=node->tree[i]->symbol;

	if(
	    (find_word(keys, model->dictionary->entry[symbol])!=0)&&
	    ((used_key==true)||
	     (find_word(aux, model->dictionary->entry[symbol])==0))&&
	    (word_exists(words, model->dictionary->entry[symbol])==false)
	    ) {
	    used_key=true;
	    break;
	}
	count-=node->tree[i]->count;
	i=(i>=(node->branch-1))?0:i+1;
    }

    return(symbol);
}

// A silly brute-force searcher for the reply string.
bool MegaHal::word_exists(HAL_DICTIONARY *dictionary, HAL_STRING word)
{
    register unsigned int i;

    for(i=0; i<dictionary->size; ++i)
	if(wordcmp(dictionary->entry[i], word)==0)
	    return(true);
    return(false);
}

// Seed the reply by guaranteeing that it contains a keyword, if one exists.
int MegaHal::seed(HAL_MODEL *model, HAL_DICTIONARY *keys)
{
    register unsigned int i;
    int symbol;
    unsigned int stop;

    /*
     *		Fix, thanks to Mark Tarrabain
     */
    if(model->context[0]->branch==0) symbol=0;
    else symbol=model->context[0]->tree[rnd(model->context[0]->branch)]->symbol;

    if(keys->size>0) {
	i=rnd(keys->size);
	stop=i;
	while(true) {
	    if(
		(find_word(model->dictionary, keys->entry[i])!=0)&&
		(find_word(aux, keys->entry[i])==0)
		) {
		symbol=find_word(model->dictionary, keys->entry[i]);
		return(symbol);
	    }
	    ++i;
	    if(i==keys->size) i=0;
	    if(i==stop) return(symbol);
	}
    }

    return(symbol);
}

// Allocate a new swap structure.
HAL_SWAP* MegaHal::new_swap(void)
{
    HAL_SWAP *list;

    list=(HAL_SWAP *)malloc(sizeof(HAL_SWAP));
    if(list==NULL) {
	error("new_swap", "Unable to allocate swap");
	return(NULL);
    }
    list->size=0;
    list->from=NULL;
    list->to=NULL;

    return(list);
}

// Add a new entry to the swap structure.
void MegaHal::add_swap(HAL_SWAP *list, char *s, char *d)
{
    list->size+=1;

    if(list->from==NULL) {
	list->from=(HAL_STRING *)malloc(sizeof(HAL_STRING));
	if(list->from==NULL) {
	    error("add_swap", "Unable to allocate list->from");
	    return;
	}
    }

    if(list->to==NULL) {
	list->to=(HAL_STRING *)malloc(sizeof(HAL_STRING));
	if(list->to==NULL) {
	    error("add_swap", "Unable to allocate list->to");
	    return;
	}
    }

    list->from=(HAL_STRING *)realloc(list->from, sizeof(HAL_STRING)*(list->size));
    if(list->from==NULL) {
	error("add_swap", "Unable to reallocate from");
	return;
    }

    list->to=(HAL_STRING *)realloc(list->to, sizeof(HAL_STRING)*(list->size));
    if(list->to==NULL) {
	error("add_swap", "Unable to reallocate to");
	return;
    }

    list->from[list->size-1].length= (uint8_t)strlen(s);
    list->from[list->size-1].word=strdup(s);
    list->to[list->size-1].length= (uint8_t)strlen(d);
    list->to[list->size-1].word=strdup(d);
}

// Read a swap structure from a file.
HAL_SWAP* MegaHal::initialize_swap(const char *filename)
{
    HAL_SWAP *list;
    FILE *file=NULL;
    char buffer[1024];
    char *from;
    char *to;

    list=new_swap();

    if(filename==NULL) return(list);

    file=fopen(filename, "r");
    if(file==NULL) return(list);

    while(!feof(file)) {

	if(fgets(buffer, 1024, file)==NULL) break;
	if(buffer[0]=='#') continue;
	from=strtok(buffer, "\t ");
	to=strtok(NULL, "\t \n#");

	add_swap(list, from, to);
    }

    fclose(file);
    return(list);
}

void MegaHal::free_swap(HAL_SWAP *swap)
{
    register int i;

    if(swap==NULL) return;

    for(i=0; i<swap->size; ++i) {
	    free_word(swap->from[i]);
	    free_word(swap->to[i]);
    }
    free(swap->from);
    free(swap->to);
    free(swap);
}

// Read a dictionary from a file.
HAL_DICTIONARY* MegaHal::initialize_list(const char *filename)
{
    HAL_DICTIONARY *list;
    FILE *file=NULL;
    HAL_STRING word;
    char *string;
    char buffer[1024];

    list=new_dictionary();

    if(filename==NULL) return(list);

    file=fopen(filename, "r");
    if(file==NULL) return(list);

    while(!feof(file)) {
	    if(fgets(buffer, 1024, file)==NULL)
            break;
	    if(buffer[0]=='#')
            continue;
	    string=strtok(buffer, "\t \n#");

	    if((string!=NULL) && (strlen(string)>0)) {
	        word.length= (uint8_t)strlen(string);
	        word.word=strdup(buffer);
            add_word(list, word);
            free(word.word);
	    }
    }

    fclose(file);
    return(list);
}

// Return a random integer between 0 and range-1.
int MegaHal::rnd(int range)
{
    return rand() % range;
}

void MegaHal::free_words(HAL_DICTIONARY *words)
{
    if(words == NULL)
        return;

    if(words->entry != NULL)
	    for(int i = 0; i < (int)words->size; ++i)
            free_word(words->entry[i]);
}

void MegaHal::free_word(HAL_STRING word)
{
    free(word.word);
}

bool MegaHal::isWordChar(uint8_t c) {
    return c && strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$#@%", c) != NULL;
}

bool MegaHal::isWordSep(uint8_t c) {
    return c && strchr("-/&'.", c) != NULL;
}

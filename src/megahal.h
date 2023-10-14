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

#pragma once
#include <string>

struct HAL_STRING {
    uint8_t length;
    char* word;
};

struct HAL_DICTIONARY  {
    uint32_t size;
    HAL_STRING* entry;
    uint16_t* index;
};

struct HAL_TREE {
    uint16_t symbol;
    uint32_t usage;
    uint16_t count;
    uint16_t branch;
    HAL_TREE** tree;
};

struct HAL_MODEL {
    uint8_t order;
    HAL_TREE* forward;
    HAL_TREE* backward;
    HAL_TREE** context;
    HAL_DICTIONARY* dictionary;
};

struct HAL_SWAP {
    uint16_t size;
    HAL_STRING* from;
    HAL_STRING* to;
};

// UTF-8 characters can be up to 4-bytes long
#define HAL_MAX_REPLY_LEN (128*4)

// fallback paths for when individual personality files don't exist
#define HAL_COMMON_SWP "common_hal.swp"
#define HAL_COMMON_BAN "common_hal.ban"
#define HAL_COMMON_AUX "common_hal.aux"
#define HAL_COMMON_TRN "common_hal.trn"

class MegaHal {
public:
    ~MegaHal();

    // Initialize various brains and files.
    // brn/trn/ban/aux/swp files with the path prefix will be loaded if they exist,
    // otherwise the fallback paths will be used (HAL_COMMON_*).
    // A .brn file will be created if none exists and a .trn was used.
    // path = file path to a .brn file without the .brn extension.
    void load_personality(const char* path);

    // Take string as input, and return allocated string as output.
    // string memory belongs to the class
	char* do_reply(char* input, bool learnFromInput);

    // Take string as input, update model but don't generate reply.
	void learn_no_reply(char* input);

    // Save the current state to a MegaHAL brain file.
	void save_model();

    // rapidly learn from a text file containing chat messages seperated by newlines
    void train(const char* path);

    // writes the model dictionary to a .dic file (same dir as the .brn) for training purposes
    void write_dictionary();

private:
    int order = 5;

    // increase for more "surprising" replies.
    // 32768 is a slow but sane max, most improvements are made below 256 iterations.
    // a low count of iterations seems to prevent the same messages being repeated with a large data set.
    int reply_iterations = 16;

    char replyBuffer[HAL_MAX_REPLY_LEN];

    HAL_DICTIONARY* words = NULL;
    HAL_MODEL* model = NULL;

    HAL_DICTIONARY* ban = NULL;
    HAL_DICTIONARY* aux = NULL;
    HAL_SWAP* swp = NULL;
    bool used_key = false;

	std::string brnpath;

    void add_aux(HAL_MODEL*, HAL_DICTIONARY*, HAL_STRING);
    void add_key(HAL_MODEL*, HAL_DICTIONARY*, HAL_STRING);
    void add_node(HAL_TREE*, HAL_TREE*, int);
    void add_swap(HAL_SWAP*, char*, char*);
    HAL_TREE* add_symbol(HAL_TREE*, uint16_t);
    uint16_t add_word(HAL_DICTIONARY*, HAL_STRING);
    int babble(HAL_MODEL*, HAL_DICTIONARY*, HAL_DICTIONARY*);
    bool boundary(char*, int);
    void capitalize(char*);
    bool dissimilar(HAL_DICTIONARY*, HAL_DICTIONARY*);
    void error(char*, char*, ...);
    float evaluate_reply(HAL_MODEL*, HAL_DICTIONARY*, HAL_DICTIONARY*);
    HAL_TREE* find_symbol(HAL_TREE*, int);
    HAL_TREE* find_symbol_add(HAL_TREE*, int);
    uint16_t find_word(HAL_DICTIONARY*, HAL_STRING);
    char* generate_reply(HAL_MODEL*, HAL_DICTIONARY*);
    void learn(HAL_MODEL*, HAL_DICTIONARY*);
    void make_words(char*, HAL_DICTIONARY*);
    HAL_DICTIONARY* new_dictionary(void);

    void upper(char*);

    void free_dictionary(HAL_DICTIONARY*);
    void free_model(HAL_MODEL*);
    void free_tree(HAL_TREE*);
    void free_word(HAL_STRING);
    void free_words(HAL_DICTIONARY*);
    void free_swap(HAL_SWAP* swap);
    void initialize_context(HAL_MODEL*);
    void initialize_dictionary(HAL_DICTIONARY*);
    HAL_DICTIONARY* initialize_list(const char*);
    HAL_SWAP* initialize_swap(const char*);
    void load_dictionary(FILE*, HAL_DICTIONARY*);
    bool load_model(const char*, HAL_MODEL*);
    void load_tree(FILE*, HAL_TREE*);
    void load_word(FILE*, HAL_DICTIONARY*);
    HAL_DICTIONARY* make_keywords(HAL_MODEL*, HAL_DICTIONARY*);
    char* make_output(HAL_DICTIONARY*);
    HAL_MODEL* new_model(int);
    HAL_TREE* new_node(void);
    HAL_SWAP* new_swap(void);
    HAL_DICTIONARY* reply(HAL_MODEL*, HAL_DICTIONARY*);
    void save_dictionary(FILE*, HAL_DICTIONARY*);
    void save_tree(FILE*, HAL_TREE*);
    void save_word(FILE*, HAL_STRING);
    int search_dictionary(HAL_DICTIONARY*, HAL_STRING, bool*);
    int search_node(HAL_TREE*, int, bool*);
    int seed(HAL_MODEL*, HAL_DICTIONARY*);
    void update_context(HAL_MODEL*, int);
    void update_model(HAL_MODEL*, int);
    bool warn(char*, char*, ...);
    int wordcmp(HAL_STRING, HAL_STRING);
    bool word_exists(HAL_DICTIONARY*, HAL_STRING);
    int rnd(int);

    bool isWordChar(uint8_t c); // is this character part of a word? TODO: unicode support
    bool isWordSep(uint8_t c); // is this character a compound word separator? ("co-op", "something.com")
    void free_everything();
};

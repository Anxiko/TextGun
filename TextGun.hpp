/*
 * TextGun.hpp
 *
 * Copyright 2016 Joaquín Monteagudo Gómez <kindos7@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

/*
    C++ library (header file)
    TextGun
    Node-Link frecuency based text learner
*/

/*
    Preprocessor
*/

/*Header guard*/
#ifndef _TEXT_GUN_H_
#define _TEXT_GUN_H_


/* Includes */

#include <list>//Linked lists
#include <utility>//Pairs
#include <map>//Maps
#include <set>//Sets
#include <string>//Strings
#include <istream>//Input stream
#include <sstream>//String stream
#include <random>//Random number generation
#include <chrono>
#include <ostream>//Writing to file
#include <istream>//Reading from file
#include <cctype>//Char functions
#include <cmath>//Hypotenuse and square root

/* Defines */

/*Constants*/

/*Macros*/

namespace TextGun
{
    /*
        Class definitions
    */

    enum class WordType : char;//Type of a word

    class Word;//Stores a word, indicates if it's special

    class FrecLink;//Array of links to nodes sorted based on their frecuencyclass FrecLink;//Array of links to nodes sorted based on their frecuency

    class WordNode;//Node for a word, frecuency and links on both directions

    class WordGraph;//Contains the WordNodes, indexed by their Word

    class ITextStream;//Provides the Words from a input stream

    class OTextStream;//Outputs words to a output stream

    class WordModel;//Model capable of learning and speaking

    class ClusterWord;//Cluster of similar words

    class ClusterRepr;//Representant of a cluster

    /*
        Function prototypes
    */

    /* UTF-8 parsing */

    /*Reading*/

    //Read a character from a string iterator, and return it in a string. Return the empty string for any errors
    std::string read_utf8_character(std::string::const_iterator &it,std::string::const_iterator e);

    //Return a string to another (purely based on size)
    bool return_utf8_string(std::string s,std::string::const_iterator &it,std::string::const_iterator e);

    /*
        Data types
     */

    /* Typedefs */

    /*Frecuency*/

    //Probability (0 to 1 frecuency)
    typedef float prob_frec;

    /* Classes */

    //Type of a word
    enum class WordType : char
    {
        START=0,//Start of text
        WORD,//Text word
        SYMBOL,//Symbol word (could not be parsed)
        L_DELIM,//Text separator that appears at the start of a word, such as an opener parenthesis ('(')
        R_DELIM,//Text separator that appears at the end of a word, such as an closer parenthesis (')')
        L_STOP,//Text stop that appears at the start of a word, such as a exclamation mark opener (¡)
        R_STOP,//Text stop that appears at the end of a word, such as a dot (!)
        INT,//Integer number (27)
        DECIMAL,//Decimal number (27.5)
        END//End of text
    };

    //Stores a word, indicates if it's special
    class Word
    {
        /*Config*/

        /*Special words*/
        public:

            //Special characters
            static const std::map<std::string,WordType> SPEC_CHAR;

            //Numeric separators
            static const std::set<std::string> NUM_SEP;

            //Text word separators
            static const std::set<std::string> TXT_SEP;

            //Words to not be altered
            static const std::set<std::string> TXT_LIT;


        /* Attributes */

        /*Word*/
        private:

            std::string s;//Word
            WordType t;//Type of the node

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            Word(const std::string &ns,WordType nt);

            //Word constructor
            Word(const std::string &ns);

            //Special node constructor
            Word(WordType nt);


        /*Operator*/
        public:

            //Less than operator, needed for mapping
            bool operator<(const Word &w) const;

            //Equality operator
            bool operator==(const Word &w) const;

            //Inequality operator
            bool operator!=(const Word &w) const;

        /* Methods */

        /*Get/set*/
        public:

            //Get

            //Get the text
            const std::string& get_text() const
            {
                return s;
            }

            //Get the type
            WordType get_type() const
            {
                return t;
            }

            //Set

            //Set as a word
            void set_text(const std::string &ns)
            {
                s=ns;
                t=WordType::WORD;
            }

            //Set as a special word
            void set_type(WordType nt)
            {
                s="";
                t=nt;
            }

            //Set type and text
            void set_all(const std::string &ns, WordType nt)
            {
                s=ns;
                t=nt;
            }

        /*Read/write to file*/
        public:

            //Write word to stream
            void write(std::ostream &o) const;

            //Read word to stream
            void read(std::istream &i);

        /*Printing*/
        public:

            //Human readable printing
            void print(std::ostream &os);

    };

    //List of links to nodes sorted based on their frecuency
    class FrecLink
    {
        /* Config */

        /*Random*/
        private:

            //Random engine
            static std::default_random_engine re;

        /* Attributes */

        /*Links*/
        private:

            //Lists of words and their frecuency
            std::list< std::pair< int,WordNode * > > words;

            //Dictionary that stores the position of each word on the list
            std::map< WordNode *,std::list< std::pair< int,WordNode * > >::iterator > dict;

            //Total number of words (sum of frec)
            int f;

            //Total number of links
            int n;

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructors
            FrecLink();

        /* Methods */

        /*Add/delete*/
        public:

            //Add a word to the list
            void add_word(WordNode &w);

        private:

            //Take an iterator to a word on the list, and return another word to swap them so that the list is still sorted. Return the same iterator if no swap is needed
             std::list< std::pair< int,WordNode * > >::iterator keep_sorted_swap(const std::list< std::pair< int,WordNode * > >::iterator &it) const;

        /*Links*/
        public:

            //Get a random word based on frecuency
            WordNode * get_rand() const;

        /*Similarity*/
        public:

             //Get the similarity between two FrecLink
            /*
                A rating between 0 (completly different) and 1 (exactly equal) determines
                how similar two frecuency dictionaries are, based on how many links they share
            */
            static prob_frec similarity_frec_link(const FrecLink &d1, const FrecLink &d2);

    };

    //Node for a word, frecuency and links on both directions
    class WordNode
    {
        /* Attributes */

        /*Links*/
        private:

            FrecLink prev,next;//Links to the wordnodes found before this one, and after this one

        /*Data*/
        public:

            //Data of this node

            Word w;//Word stored on this node
            int f;//Frecuency of this word

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            WordNode(const Word &nw);

        /* Methods */

        /*Links*/
        public:

            //Add a link

            //Add a link to a previous word
            void add_prev(WordNode *w);

            //Add a link to a next word
            void add_next(WordNode *w);

            //Get a random word

            //Get a random previous word
            WordNode * get_prev() const;

            //Get a random next word
            WordNode * get_next() const;

        /*Word*/
        public:

            //Increase frecuency
            void inc_frec();

            //Get word
            const Word& get_word() const
            {
                return w;
            }

        /*Similarity*/
        public:

            //Similarity between two nodes
            /*
                The similarity between two word nodes is calculated as the product
                between the similarity between their forward dictionaries, and the similarity between their backward dictionaries
            */

            static prob_frec similarity_node(const WordNode &n1, const WordNode &n2);

        /*Representant*/
        public:

            //Generate a representant for this node
            ClusterRepr repr() const;
    };


    //Contains the WordNodes, indexed by their Word
    class WordGraph
    {
        /* Config */

        /*Friends*/

            friend class WordModel;

        /* Attributes */

        /*Nodes*/
        private:

            std::map< Word,WordNode > nodes;//Nodes indexed by their word
            int n;//Number of nodes

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructor
            WordGraph();

        /* Methods */

        /*Nodes*/
        public:

            //Check if a word exists (as a node in the graph)
            bool check_word(const Word &w) const;

            //Add a word to the node, increase its frecuency if it exists
            void add_word(const Word &w);

            //Get a node by pointer, nullptr if not found
            WordNode* get_node(const Word &w);

            //Get a node by pointer to const, nullptr if not found
            const WordNode* get_node(const Word &w) const;

        /*Links*/
        public:

            //Add a link between two nodes
            void add_link(const Word &prev, const Word &next);

        /*Similarity*/
        public:

            //Similarity between two nodes. Always zero if either word is not found
            prob_frec similarity_word(const Word &w1, const Word &w2) const;
    };

    //Provides the Words from a input stream
    class ITextStream
    {
        /* Config */

        /*Types*/
        private:

            enum class StreamState
            {
                START,//Feed start word
                TEXT,//Feed text words from the stream
                END,//Feed end word
                EMPTY//Stream is empty, will not provide more words
            };

        /*Defaults*/
        private:

            //Word to be returned if an error arises during reading
            static const Word DEF_ERR_WORD;

        /* Attributes */

        /*Stream*/
        private:

            //Input stream
            std::istream &is;

            //Status of the stream
            StreamState status;

            //Next word to be fed
            std::list<Word> nw;

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            ITextStream(std::istream &nis);

        /* Methods */

        /*Stream*/
        public:

            //Loads the next words from the stream, returns if the stream is ready. Must be called after every read
            bool has_words();

            //Return the last word read from the stream
            Word read();

        /*Parsing*/
        private:

            /*Read a word, or a part of it. Return true if you read what expected, false otherwise. Return the readen word*/

            //Fill the queue with words from a text separated by whitespàce
            bool read_word(std::string s);

            //Read whitespace
            bool read_WS(std::string::const_iterator &it,std::string::const_iterator e,Word &w);

            //Read a left stop
            bool read_L_STOP(std::string::const_iterator &it,std::string::const_iterator e,Word &w);

            //Read a left delimiter
            bool read_L_DELIM(std::string::const_iterator &it,std::string::const_iterator e,Word &w);

            //Read content
            bool read_CONTENT(std::string::const_iterator &it,std::string::const_iterator e,Word &w);

            //Read a right delimiter
            bool read_R_DELIM(std::string::const_iterator &it,std::string::const_iterator e,Word &w);

            //Read a right stop
            bool read_R_STOP(std::string::const_iterator &it,std::string::const_iterator e,Word &w);
    };

    //Outputs words to a output stream
    class OTextStream
    {

        /* Attributes */

        /*Stream*/
        private:

            //Output stream
            std::ostream &os;

            //Status of the stream, previously printed word
            WordType state;

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            OTextStream(std::ostream &nos);

        /* Methods */

        /*Write*/
        public:

            //Write a word to the stream
            bool write(const Word &w);
    };

    //Representant of a cluster
    class ClusterRepr
    {
        /* Attributes */

        /*Dictionaries*/
        private:

            std::map<WordNode *, int> prev,next;//Forward and backward frecuencies for each node

            int f;//Sum of frecuencies

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructor
            ClusterRepr() = default;

            //Move constructor
            ClusterRepr(ClusterRepr &&mv);

        /* Methods */

        /*Dictionary*/
        public:

            //Add the links from another representant to this one
            void add(const ClusterRepr &repr);

            //Similarity between two representants (same formula as words)
            static void similarity_repr(const ClusterRepr &a, const ClusterRepr &b);

        /*Swapping*/
        private:

            void swap(ClusterRepr &&mv);
    };

    //Cluster of similar words
    class ClusterWord
    {
        /* Config */

        /*Friends*/

            friend WordNode;

        /*UID*/
        public:

            typedef unsigned int cuid;//Type of the cluster's ID

        private:

            static cuid guid;//Generator of unique identifiers

        /*Cache*/
        private:

            static std::map<std::pair<cuid,cuid>,prob_frec> cache_simil;//Cached similarity values

        /* Attributes */

        /*Set*/
        public:

            std::set<Word> words;//Words in this cluster
            cuid id;//Identifier for this cluster

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructor
            ClusterWord();

            //Constructor with initial word
            ClusterWord(const Word &w);

        /* Methods */

        /*Clustering*/
        public:

            //Similarity

            //Similarity between two clusters
            /*
                The similarity between clusters is calculated
                using the similarity between the words of each clusters,
                doing the weighted arithmetic average.

                The calculus is done for each pair of words,
                the value is the similarity between words,
                the weight is the hypotenuses calculus (the sides of the triangles are the frecuencies of each word)
            */
            static prob_frec similarity_cluster(const ClusterWord &c1, const ClusterWord &c2, const WordGraph &g);

            //Join a cluster into this one
            void join_cluster(const ClusterWord &c);

        /*IDs and caching*/
        private:

            //Get a valid ID
            static cuid get_id();
    };

    //Model capable of learning and speaking
    class WordModel
    {
        /* Attributes */

        /*Nodes*/
        private:

            WordGraph graph;//Graph to be trained and to generate sentences
            std::list<ClusterWord> clusters;//List of clusters of similar words

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructor
            WordModel();

        /* Methods */

        /*Learn*/
        public:

            //Learn from a text stream
            void learn(ITextStream &ts);

        /*Speak*/
        public:

            //Generate a line using the model
            void think(OTextStream &ots);

        /*Similarity*/
        public:

            //Similarity between two nodes. Always zero if either word is not found
            prob_frec similarity_word(const Word &w1, const Word &w2) const;

            //Generate the clusters
            void clustering();

        /*Read/write to file*/
        public:

            //Write to file
            void write(std::ostream &o) const;

            //Read from file
            void read(std::istream &i);
    };
}//End of namespace

//End of library
#endif // _TEXT_GUN_H_

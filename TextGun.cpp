/*
 * TextGun.cpp
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
    C++ library (source file)
    TextGun
    Node-Link frecuency based text learner
*/

/*
    Preprocessor
*/

/* Includes */

//Header file
#include "TextGun.hpp"

namespace TextGun
{
    /*
        Init
    */

    /* Word */

    /*Config*/

    //Special characters
    const std::map<std::string,WordType> Word::SPEC_CHAR
    {
        {",",WordType::R_DELIM},{";",WordType::R_DELIM},{":",WordType::R_DELIM},{")",WordType::R_DELIM},//Right delimiters
        {"(",WordType::L_DELIM},//Left delimiters
        {".",WordType::R_STOP},{"!",WordType::R_STOP},{"?",WordType::R_STOP},//Right delimiters
        {"\u00A1",WordType::L_STOP},{"\u00BF",WordType::L_STOP}//Left delimiters
    };

    //Numeric separators
    const std::set<std::string> Word::NUM_SEP
    {
        "'",".",","
    };

    //Text word separators
    const std::set<std::string> Word::TXT_SEP
    {
        "'"
    };

    //Words to not be altered
    const std::set<std::string> Word::TXT_LIT
    {
        std::string("I")
    };

    /* FrecLink */

    std::default_random_engine FrecLink::re(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));//Random engine

    /* ITextStream */

    //Word to be returned if an error arises during reading
    const Word ITextStream::DEF_ERR_WORD(WordType::END);

    /* ClusterWord */

    /*UID*/
    ClusterWord::cuid ClusterWord::guid=0u;//Generator of unique identifiers

    /*Cache*/
    std::map<std::pair<ClusterWord::cuid,ClusterWord::cuid>,prob_frec> ClusterWord::cache_simil;//Cached similarity values

    /*
            Functions
    */

    /* UTF-8 parsing */

    /*Reading*/

    //Read a character from a string iterator, and return it in a string. Return the empty string for any errors
    std::string read_utf8_character(std::string::const_iterator &it,std::string::const_iterator e)
    {
        if (it==e)//If at the end
        {
            return std::string();//Empty string
        }
        else//Read the first byte, decide the length of the character
        {
            //Make a copy of the iterator, to recover it in case of error
            std::string::const_iterator cpy=it;

            //There's at least one byte. If we read it, we'll know the length of the character to read
            unsigned char first_byte=static_cast<unsigned char>(*it);//Read it as a byte! We're gonna do bitwise operations.

            if (first_byte&0x80)//If the 8th byte is set to 1, the character is multibyte. We still have to figure out the length
            {
                /*
                    The first byte indicates how many bytes is the character composed of.
                    The 8th byte is 1. Then follow as many 1s as bytes has the character, then zero.
                    110_____ = 2 bytes, one's alredy read
                    1110____ = 3 bytes, one's alredy read
                    11110___ = 4 bytes, one's alredy read

                    All the following character must have the 10______ structure, so we'll check for that as well.
                */

                //First, determine the size
                int n=0;//Bytes left to read
                for (unsigned char bit_finder=0x40;bit_finder&first_byte;bit_finder=bit_finder>>1)//Iterate throught the bits, starting at the 7th until the 0 is found
                    ++n;//One more bit read

                //Now, to read the the bytes
                std::string rv(1,*(it++));//String with the bytes. Create it with the first

                for (;n>0 && it!=e;--n,++it)//Read all the bytes
                {
                    unsigned char byte=static_cast<unsigned char>(*it);//Read the byte

                    if((byte&0xC0) != 0x80)//Check the byte is in the format 0x80, or 10______
                        break;//ERROR! Byte is not in the expected format. Stop reading

                    rv+=*it;//Add the byte to the string! It's in the correct format
                }

                //We're out of the loop. Check why
                if (n)//Are there any bytes left to read? If so, an error happened
                {
                    it=cpy;//Restore the iterator
                    return std::string();//Return the empty string
                }
                else//No bytes left, everything went correctly
                {
                    return rv;//Return the string with the bytes
                }
            }
            else//Character is one byte
            {
                return std::string (1,*(it++));//String to return, has the first byte, which is the character
            }
        }
    }

    //Return a string to another (purely based on size). Check that it does not go past the begin. Return true if everything went correctly, false otherwise
    bool return_utf8_string(std::string s,std::string::const_iterator &it,std::string::const_iterator b)
    {
        if (std::distance(b,it)<static_cast<signed int>(s.size()))//Can't return everything!
            return false;//Report the error
        else
        {
            std::advance(it,-s.size());//Return the thing string
            return true;//Everything went correctly
        }
    }

    /*
        Word
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    Word::Word(const std::string &ns,WordType nt)
    :s(ns),t(nt)
    {}

    //Word constructor
    Word::Word(const std::string &ns)
    :Word(ns,WordType::WORD)
    {}

    //Special node constructor
    Word::Word(WordType nt)
    :Word("",nt)
    {}

    /*Operator*/

    //Less than operator, needed for mapping
    bool Word::operator<(const Word &w) const
    {
        //Compare first by type, then by text

        if (t!=w.t)
            return t<w.t;
        return s<w.s;
    }

    //Equality operator
    bool Word::operator==(const Word &w) const
    {
        return (t==w.t)&&(s==w.s);
    }

    //Inequality operator
    bool Word::operator!=(const Word &w) const
    {
        return !operator==(w);
    }

    /* Methods */

    /*Read/write to file*/

    //Write word to stream
    void Word::write(std::ostream &o) const
    {
        //Write the type of the word
        o.write(reinterpret_cast<const char *>(&t),sizeof(char));

        //Write the number of bytes of the string
        std::string::size_type sz=s.size();
        o.write(reinterpret_cast<const char *>(&sz),sizeof(int));

        //Write the string
        o.write(s.c_str(),sz);
    }

    //Read word to stream
    void Word::read(std::istream &i)
    {
        //Read the type of the word
        i.read(reinterpret_cast<char *>(&t),sizeof(char));

        //Read the number of bytes
        std::string::size_type sz;
        i.read(reinterpret_cast<char *>(&sz),sizeof(std::string::size_type));

        //Create a buffer
        char *str=new char[sz+1];

        //Read to buffer
        i.read(str,sz);
        str[sz]='\0';//Ensure null terminated

        //Put into string
        s=std::string(str);

        //Delete buffer
        delete[] str;
    }

    /*Printing*/

    //Human readable printing
    void Word::print(std::ostream &os)
    {
        switch (t)
        {
            case WordType::START:
            {
                os<<"{START}";
                break;
            }

            case WordType::WORD:
            {
                os<<"{WORD: "<<s<<'}';
                break;
            }

            case WordType::SYMBOL:
            {
                os<<"{SYMBOL: "<<s<<'}';
                break;
            }

            case WordType::L_DELIM:
            {
                os<<"{L_DELIM: "<<s<<'}';
                break;
            }

            case WordType::R_DELIM:
            {
                os<<"{R_DELIM: "<<s<<'}';
                break;
            }

            case WordType::L_STOP:
            {
                os<<"{L_STOP: "<<s<<'}';
                break;
            }

            case WordType::R_STOP:
            {
                os<<"{R_STOP: "<<s<<'}';
                break;
            }

            case WordType::INT:
            {
                os<<"{INT: "<<s<<'}';
                break;
            }

            case WordType::DECIMAL:
            {
                os<<"{DECIMAL: "<<s<<'}';
                break;
            }

            case WordType::END:
            {
                os<<"{END}";
                break;
            }

            default:
            {
                os<<"{ERROR}";
                break;
            }
        }
    }

    /*
        FrecLink
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Default constructors
    FrecLink::FrecLink()
    :words(),dict(),f(0),n(0)
    {}

    /* Methods */

    /*Add/delete*/

    //Add a word to the list
    void FrecLink::add_word(const Word &w)
    {
        ++f;//Increase the frecuency of total links

        //Check if the word is on the list
        if (dict.find(w)==dict.end())//Not found
        {
            //Insert it at the end of the list with frec=1 (since it's the first time this word's been seen)
            words.emplace_back(1,w);
            //Add the word to the dictionary
            dict[w]=std::prev(words.end());//Save the iterator to the last element of the list, where the word is now saved

            ++n;//A new node on the list
        }
        else//Found
        {
            std::list< std::pair< int,Word > >::iterator pos=dict[w];//Get the position of the pair
            ++(pos->first);//Increment the frecuency by one

            //Find the new position on the list for this element
            std::list< std::pair< int,Word > >::iterator new_pos=keep_sorted_swap(pos);

            //Swap positions, only if they're different
            if (pos!=new_pos) words.splice(new_pos,words,pos);
        }
    }

    //Take an iterator to a word on the list, and return another word to swap them so that the list is still sorted. Return the same iterator if no swap is needed
    std::list< std::pair< int,Word > >::iterator FrecLink::keep_sorted_swap(const std::list< std::pair< int,Word > >::iterator &pos) const
    {
        std::reverse_iterator< std::list< std::pair< int,Word > >::iterator > rev_it(pos);//Reverse operator to start looking at one upper from this one
        /*
          std::advance(rev_it,1);//Advance the iterator by one, no longer looking at the same element, but at the upper one. Iterator's valid because pos!=begin()
          No need to do this! Given the way reverse iterators work, rev_it is using pos as its base(), so it's alredy pointing at the next value
        */

        while (rev_it!=words.rend())//Look while there are still elemets to iterate
        {
            if((rev_it->first) >= (pos->first))//Find one to switch for
                break;//Stop the search
            ++rev_it;//If nothing was found, continue searching
        }

        /*
            Swap the position between pos and the new found one (rev_it).
            Get the forward iterator, which points to the previous element, using base().
            If no value was found, rev_it==rend(), which means rev_it.base()==begin(),
            so it'll be swapped to the top where it belongs (since no value being found means this is the biggest one).
            If a value was found, then base() contains the previous value,
            so it'll be swapped right before the value found, keeping things sorted.
            NOTE: getting base() from a reverse iterator will always be valid if the reverse iterator was valid,
            keeping in mind that base() of rbegin() is end(), which is a valid iterator that cannot be dereferenced.
        */

        return rev_it.base();
    }

    /*Links*/

    //Get a random word based on frecuency
    Word FrecLink::get_rand() const
    {
        //Create the RNG to use it with the engine
        std::uniform_int_distribution<> dt(0,f-1);

        //Random number generated
        int n=dt(re);

        //Navigate through the links until the goal number is met
        for (std::pair< int,Word > w : words)
        {
            n-=w.first;//Decrease the goal by this word's frecuency

            if(n<0)//If the goal is met, return this word
                return w.second;
        }

        //If no word is found, an error just happened
        return Word(WordType::END);//Should throw an exception, by the time being the END world will be returned. Note that the word END is valid
    }

    /*Read/write to file*/

    //Write word to stream
    void FrecLink::write(std::ostream &o) const
    {
        //Write the number of entries
        o.write(reinterpret_cast<const char *>(&n),sizeof(int));
        //Write the sum of the frecuencies
        o.write(reinterpret_cast<const char *>(&f),sizeof(int));

        //Write the list
        for(const std::pair< int,Word > &p : words)
        {
            //Write the frec
            o.write(reinterpret_cast<const char *>(&p.first),sizeof(int));

            //Write the word
            p.second.write(o);
        }
    }

    //Read word to stream
    void FrecLink::read(std::istream &i)
    {
        //Read number of entries
        n=0;//Set to zero in case of fail reading
        i.read(reinterpret_cast<char *>(&n),sizeof(int));
        //Read the sum of the frecuencies
        i.read(reinterpret_cast<char *>(&f),sizeof(int));

        //Iterator to insert
        auto it=words.begin()==words.end()?words.begin():std::prev(words.end());//This list should always be empty, just to make sure, start at the end

        //Read the list
        int iters=n;
        while(iters-->0)//Read all the entries
        {
            //Read the frec
            int word_frec=0;
            i.read(reinterpret_cast<char *>(&word_frec),sizeof(int));

            //Read the word
            Word w("");
            w.read(i);

            //Insert the word on the list
            it=words.emplace(it,word_frec,w);

            //Insert the word on the map
            dict[w]=it++;//Increment the iterator after inserting, as to keep inserting after the end
        }
    }

    /*Similarity*/

    //Get the similarity between two FrecLink
    prob_frec FrecLink::similarity_frec_link(const FrecLink &d1, const FrecLink &d2)
    {
        /*
            The similarity rating (on the [0,1] set) is a weighted average
            of the similarities of each individual link that belongs to either set (union of sets).

            The value is the similarity between links (3 nodes, two nodes being compared, and a third one that is conncted to the other 2),
            which is also on the [0,1] range.

            The wieghts are the square root of the sum of the squared frecuencies,
            (if it were a triangle, the hypotenuse would be the weight, and the frecuencies would be the other sides.

             The similarity between two links uses the frecuencies between each of the two nodes to the thrird one (two frecuencies).
             It is simply the harmonic mean divided by the arithmetic mean
        */

        /*
            Since we only have to process each link once, we have to make sure we don't process duplicates.
            We don't have to worry about that when we process the first dictionary, but we have to check that it's not in the first when we go through the second.
            Because of that, we process the biggest dictionary first
        */

        //Set the first and last dictionary (bigger and smaller)
        const FrecLink *ptr_d1, *ptr_d2;//Pointers to the dictionaries (bigger one is d1)

        if (d1.n>d2.n)//d1 is the bigger one
        {
            ptr_d1=&d1;
            ptr_d2=&d2;
        }
        else//d2 is the bigger one
        {
            ptr_d1=&d2;
            ptr_d2=&d1;
        }

        //Iterate over the dictionaries

        prob_frec ponderated_sum=0;//Ponderated sum of the similarities (dividend)
        prob_frec weight_sum=0;//Sum of weights (divider)


        //Iterate over the first dictionary
        for  (const std::pair< int,Word > &word : ptr_d1->words)//Iterate over the links of the bigger dictionary, get a pair of the word and its frecuency
        {
            prob_frec f1=static_cast<prob_frec>(word.first)/static_cast<prob_frec>(ptr_d1->f);//The first frecuency can be extracted directly from the pair
            prob_frec f2=0;//Second frecuency, not yet known (will remain at 0 if not found)

            //Look for the link using the word on the other dictionary
             std::map< Word,std::list< std::pair< int,Word > >::iterator >::const_iterator it = ptr_d2->dict.find(word.second);
             if (it!=ptr_d2->dict.cend())//Link exists, get the frecuency
                f2=static_cast<prob_frec>(it->second->first)/static_cast<prob_frec>(ptr_d2->f);//Get the frecuency from the iterator to the the list

            //Might want to check that f1+f2!=0 here? Shouldn't happen, since it being on the bigger dictionary implies f1 is not zero

            //Calculate the value and the weight
            prob_frec weight=std::hypot(f1,f2);//=(f1*f1+f2*f2)^(1/2), hypotenuse calculus
            prob_frec simil=(2*std::sqrt(f1*f2))/(static_cast<prob_frec>(f1+f2));//Similarity degree between links, between 0 and 1

            //Add these values to the total
            ponderated_sum+=weight*simil;//Add the ponderated similarity to the sum
            weight_sum+=weight;//Add the weight to the sum
        }

        //Iterate over the second dictionary
        for  (const std::pair< int,Word > &word : ptr_d2->words)//Iterate over the links of the smaller dictionary, get a pair of the word and its frecuency
        {
            //Before anything, check if this link has already being processed (it is in the bigger dictionary)
            std::map< Word,std::list< std::pair< int,Word > >::iterator >::const_iterator it = ptr_d1->dict.find(word.second);

            if (it!=ptr_d1->dict.cend())//If it's in the other dictionary, skip it; it's already been processed
                continue;

            //If we're still here, the current word is only in this dictionary, and not in the other

            //Since one of the frecuencies is 0, the weight is just the other one, and the similarity is zero
            //ponderated_sum+=0;//No need to add anything, since the similarity is 0
            weight_sum+=static_cast<prob_frec>(word.first)/static_cast<prob_frec>(ptr_d2->f);
        }

        return ponderated_sum/weight_sum;//Return the similarity between dictionaries, division between the sum of the ponderated link smilarities and the weights
    }


    /*
        WordNode
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    WordNode::WordNode(const Word &nw)
    :prev(),next(),w(nw),f(1)
    {}

    /* Methods */

    /*Links*/

    //Add a link

    //Add a link to a previous word
    void WordNode::add_prev(const Word &w)
    {
        prev.add_word(w);
    }

    //Add a link to a next word
    void WordNode::add_next(const Word &w)
    {
        next.add_word(w);
    }

    //Get a random word

    //Get a random previous word
    Word WordNode::get_prev() const
    {
        return prev.get_rand();
    }

    //Get a random next word
    Word WordNode::get_next() const
    {
        return next.get_rand();
    }

    /*Word*/

    //Increase frecuency
    void WordNode::inc_frec()
    {
        ++f;
    }

    /*Similarity*/

    //Similarity between two nodes
    prob_frec WordNode::similarity_node(const WordNode &n1, const WordNode &n2)
    {
        //Calculate the forward and backward similarities, and return their product
        return FrecLink::similarity_frec_link(n1.prev,n2.prev)*FrecLink::similarity_frec_link(n1.next,n2.next);
    }

    /*Read/write to file*/

    //Write to file
    void WordNode::write(std::ostream &o) const
    {
        //Write the word of the node
        w.write(o);

        //Write the frecuency
        o.write(reinterpret_cast<const char *>(&f),sizeof(int));

        //Write the links to previous words
        prev.write(o);

        //Write the links to next words
        next.write(o);
    }

    //Read from file
    void WordNode::read(std::istream &i)
    {
        //Read the word of the node
        w.read(i);

        //Read the frecuency
        i.read(reinterpret_cast<char *>(&f),sizeof(int));

        //Read the links to previous words
        prev.read(i);

        //Read the links to next words
        next.read(i);
    }

    /*
        WordGraph
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Default constructor
    WordGraph::WordGraph()
    :nodes(),n(0)
    {}

    /* Methods */

    /*Nodes*/

    //Check if a word exists (as a node in the graph)
    bool WordGraph::check_word(const Word &w) const
    {
        return nodes.find(w)!=nodes.cend();
    }

    //Add a word to the node, increase its frecuency if it exists
    void WordGraph::add_word(const Word &w)
    {
        auto it=nodes.find(w);

        if (it==nodes.end())//Add if not found
        {
            nodes.emplace(w,w);
            ++n;
        }
        else//If found, increase
            it->second.inc_frec();
    }

    //Get a node by pointer, nullptr if not found
    WordNode* WordGraph::get_node(const Word &w)
    {
        if(check_word(w))
        {
            WordNode &node=nodes.at(w);
            return &node;
        }
        return nullptr;
    }

    //Get a node by pointer, nullptr if not found
    const WordNode* WordGraph::get_node(const Word &w) const
    {
        if(check_word(w))
        {
            const WordNode &node=nodes.at(w);
            return &node;
        }
        return nullptr;
    }

    /*Links*/

    //Add a link between two nodes
    void WordGraph::add_link(const Word &prev, const Word &next)
    {
        //Assuming both nodes alredy exist

        //Add link prev -> next
        nodes.at(prev).add_next(next);
        nodes.at(next).add_prev(prev);
    }

    //Similarity between two nodes. Always zero if either word is not found
    prob_frec WordGraph::similarity_word(const Word &w1, const Word &w2) const
    {
        //First, get the nodes of these words
        const WordNode *n1=get_node(w1),*n2=get_node(w2);

        //If both nodes were found, compute and return their similarity
        if (n1&&n2)
            return WordNode::similarity_node(*n1,*n2);

        //If either of the pointers was null, word was not found on the graph. Return zero
        return 0;
    }

    /*Read/write to file*/

    //Write to file
    void WordGraph::write(std::ostream &o) const
    {
        //Write the number of words
        o.write(reinterpret_cast<const char *>(&n),sizeof(int));

        //Write all the nodes
        for(const std::pair< const Word, const WordNode > &n : nodes)
            n.second.write(o);
    }

    //Read from file
    void WordGraph::read(std::istream &i)
    {
        //Read the number of words
        n=0;
        i.read(reinterpret_cast<char *>(&n),sizeof(int));

        //Read the words
        int iters=n;
        while(iters-->0)//Read all the words
        {
            //Node to read
            WordNode wn(Word(""));

            //Read the node
            wn.read(i);

            //Insert the node on the map, indexed by its word
            nodes.insert(std::pair< Word,WordNode >(wn.get_word(),wn));
        }
    }

    /*
        ITextStream
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    ITextStream::ITextStream(std::istream &nis)
    :is(nis),status(StreamState::START),nw()
    {}

    /* Methods */

    /*Stream*/

    //Loads the next words from the stream, returns if the stream is ready. Must be called after every read
    bool ITextStream::has_words()
    {
        switch(status)
        {
            //Load the start word, switch to text
            case StreamState::START:
            {
                nw.push_back(Word(WordType::START));
                status=StreamState::TEXT;
                return true;
            }

            //Load a text word, if it fails, switch to the end
            case StreamState::TEXT:
            {
                std::string s;//String to read
                if (!nw.empty())//If the buffer isn't empty
                    return true;//It has words
                else if (is>>s)//Or it can be filled again
                {
                    read_word(s);//Fill it again
                    return true;//It still has words
                }
                else
                {
                    //If the word could not be read, let the flow reach to the next case. Do not break or return
                    status=StreamState::END;
                }
            }

            //End of text, stream is now empty
            case StreamState::END:
            {
                nw.push_back(Word(WordType::END));
                status=StreamState::EMPTY;
                return true;
            }

            //When the stream is empty or on a invalid state, it has no word
            case StreamState::EMPTY:
            default:
                return false;
        }
    }

    //Return the last word read from the stream
    Word ITextStream::read()
    {
        if (nw.empty())//If it's empty, error
            return DEF_ERR_WORD;
        else//Return the front, and pop it
        {
            auto rv=nw.front();
            nw.pop_front();
            return rv;
        }
    }

    /*Parsing*/

    //Fill the queue with words from a text separated by whitespàce
    bool ITextStream::read_word(std::string s)
    {
        Word rv("");//Values returned from the functions

        std::list<Word> words;//All words extracted from the string
        auto it=s.cbegin();//Iterator, start at begin
        auto e=s.cend();//End

        //Try to read all L_STOP. May be none
        while(read_L_STOP(it,e,rv))
            words.push_back(rv);

        //Try to read all L_DELIM. May be none
        while(read_L_DELIM(it,e,rv))
            words.push_back(rv);

        //Try to read a content word. If this fails, the whole string is turned into a symbol
        if (read_CONTENT(it,e,rv))//Text was found, check that it wasn't a symbol
        {
            if (rv.get_type()!=WordType::SYMBOL)
            {
                //Correct word
                words.push_back(rv);
            }
            else
            {
                //Symbol
                nw.push_back(Word(s,WordType::SYMBOL));
                return false;//Indicate that it's a symbol
            }
        }
        //If no text was found, go on without problems

        //Try to read all R_DELIM. May be none
        while(read_R_DELIM(it,e,rv))
            words.push_back(rv);

        //Try to read all R_STOP. May be none
        while(read_R_STOP(it,e,rv))
            words.push_back(rv);

        //All string must be consumed by now. Otherwise, the whole string's a symbol
        if (it==e)
        {
            //String consumed

            //Move all words from this list to the end of the stream's list
            nw.splice(nw.end(),words);
            return true;//Reading succesful
        }
        else
        {
            //All reading is done, but there's still string left
            nw.push_back(Word(s,WordType::SYMBOL));
            return false;//Indicate that it's a symbol

        }
    }


    //Read whitespace (omit, do not return)
    bool ITextStream::read_WS(std::string::const_iterator &it,std::string::const_iterator e,Word &)
    {
        bool rv=false;//True if any ws was read
        std::string::const_iterator cpy=it;//Copy the start iterator

        while (it!=e)//Until the end is reached
        {
            std::string c=read_utf8_character(it,e);//Read a character string

            if (c==" "||c=="\t")//Space or tab
            {
                rv=true;//At least one is read
            }
            else//If it wasn't
            {
                return_utf8_string(c,it,cpy);//Return the last character string
                break;//Stop reading
            }
        }

        return rv;
    }

    //Read a left stop
    bool ITextStream::read_L_STOP(std::string::const_iterator &it,std::string::const_iterator e,Word &w)
    {
        std::string::const_iterator cpy=it;//Copy the start iterator

        if (it!=e)//If not at the end
        {
            std::string c=read_utf8_character(it,e);//Read a one character string
            auto item=Word::SPEC_CHAR.find(c);//Look for the chararcter
            if (item!=Word::SPEC_CHAR.end() && item->second == WordType::L_STOP)//If the character was found, and it was a L_STOP
            {
                w.set_all(c,WordType::L_STOP);//Set the word
                return true;//Read a word
            }
            else//Character not detected, return it
            {
                return_utf8_string(c,it,cpy);
            }
        }

        return false;//Did not read a word
    }

    //Read a left delimiter
    bool ITextStream::read_L_DELIM(std::string::const_iterator &it,std::string::const_iterator e,Word &w)
    {
        std::string::const_iterator cpy=it;//Copy the start iterator

        if (it!=e)//If not at the end
        {
            std::string c=read_utf8_character(it,e);//Read a one character string
            auto item=Word::SPEC_CHAR.find(c);//Look for the next chararcter
            if (item!=Word::SPEC_CHAR.end() && item->second == WordType::L_DELIM)//If the character was found, and it was a L_DELIM
            {
                w.set_all(c,WordType::L_DELIM);//Set the word
                return true;//Read a word
            }
            else//Character not detected, return it
            {
                return_utf8_string(c,it,cpy);
            }
        }
        return false;//Did not read a word
    }

    //Read content
    bool ITextStream::read_CONTENT(std::string::const_iterator &it,std::string::const_iterator e,Word &w)
    {
        //Text to read
        std::string word;

        //Word to be returned by auxiliary functions
        Word rv("");

        //Type of the word currently been set
        WordType t=WordType::START;
        bool trans=false;//Transitioning between states

        //Until the end has been reached
        while(it!=e)
        {
            //Check if it's the end of the word
            std::string::const_iterator cpy=it;//Make a copy of the iterator, as to not modify it
            if (read_R_DELIM(cpy,e,rv)||read_R_STOP(cpy,e,rv))//If you find a right stop or delimiter, stop reading words
                break;

            //Try to read a character
            std::string c=read_utf8_character(it,e);
            if (c.empty())//Is it empty? Some error must've happened
            {
                w.set_all(word+std::string(it,e),WordType::SYMBOL);//Everything's a symbol
                return true;
            }
            else//A character was read, try to parse it
            {
                switch(t)//Switch based on type
                {
                    case WordType::START:
                    {
                        //Try to detect the type of word

                        if//Text word
                        (
                            (Word::TXT_SEP.find(c)!=Word::TXT_SEP.end())//Text separator
                            ||
                            (//Character is a letter
                                (c.size()==1)
                                &&
                                (std::isalpha(c[0]))
                            )
                        )
                            t=WordType::WORD;//Detect it as a word

                        else if(c.size()==1&&std::isdigit(c[0]))//Integer
                            t=WordType::INT;

                        break;
                    }

                    case WordType::WORD://Reading a word
                    {
                        if
                        (
                            !//Not a valid character
                            (
                                (Word::TXT_SEP.find(c)!=Word::TXT_SEP.end())//Text separator
                                ||
                                (//Character is a letter
                                    (c.size()==1)
                                    &&
                                    (std::isalpha(c[0]))
                                )
                            )
                        )
                            t=WordType::SYMBOL;//Turn the word into a symbol
                        break;
                    }

                    case WordType::INT://Integer
                    {
                        if (Word::NUM_SEP.find(c)!=Word::NUM_SEP.end())//Number separator
                        {
                            trans=true;//Transition to decimal. If this flag is on at the end, we know the transition didn't end
                            t=WordType::DECIMAL;
                        }
                        else if (!(c.size()==1&&std::isdigit(c[0])))//If not a digit
                            t=WordType::SYMBOL;
                        break;
                    }

                    case WordType::DECIMAL://Decimal number
                    {
                        if (c.size()==1&&std::isdigit(c[0]))//If it's a digit
                        {
                            //No longer transitioning
                            trans=false;
                        }
                        else//Not a digit, turn to symbol
                        {
                            t=WordType::SYMBOL;
                        }

                        break;
                    }

                    default://No need to do anything specific for a symbol
                        break;
                }
                word+=c;//Add the character to the word
            }
        }

        //We're done. Check the type
        if(trans)//If caught midtransitioning, degenerate to symbol
            t=WordType::SYMBOL;

        //Check if anything's been read
        if(t==WordType::START)
            return false;//Report it

        //All set. Return the word
        w.set_all(word,t);
        return true;
    }

    //Read a left stop
    bool ITextStream::read_R_DELIM(std::string::const_iterator &it,std::string::const_iterator e,Word &w)
    {
        std::string::const_iterator cpy=it;//Copy the start iterator

        if (it!=e)//If not at the end
        {
            std::string c=read_utf8_character(it,e);//Read a one character string
            auto item=Word::SPEC_CHAR.find(c);//Look for the next chararcter
            if (item!=Word::SPEC_CHAR.end() && item->second == WordType::R_DELIM)//If the character was found, and it was a L_DELIM
            {
                w.set_all(c,WordType::R_DELIM);//Set the word
                return true;//Read a word
            }
            else//Character not detected, return it
            {
                return_utf8_string(c,it,cpy);
            }
        }
        return false;//Did not read a word
    }

    //Read a left stop
    bool ITextStream::read_R_STOP(std::string::const_iterator &it,std::string::const_iterator e,Word &w)
    {
        std::string::const_iterator cpy=it;//Copy the start iterator

        if (it!=e)//If not at the end
        {
            std::string c=read_utf8_character(it,e);//Read a one character string
            auto item=Word::SPEC_CHAR.find(c);//Look for the next chararcter
            if (item!=Word::SPEC_CHAR.end() && item->second == WordType::R_STOP)//If the character was found, and it was a L_DELIM
            {
                w.set_all(c,WordType::R_STOP);//Set the word
                return true;//Read a word
            }
            else//Character not detected, return it
            {
                return_utf8_string(c,it,cpy);
            }
        }
        return false;//Did not read a word
    }

    /*
        OTextStream
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    OTextStream::OTextStream(std::ostream &nos)
    :os(nos),state(WordType::START)
    {}

    /* Methods */

    /*Write*/

    //Write a word to the stream
    bool OTextStream::write(const Word &w)
    {
        //Check stream's state
        if (state==WordType::END)//If the stream's closed
        {
            if (!(w.get_type()==WordType::END || w.get_type()==WordType::START))//If we aren't closing, or reopening the stream
                return false;//Error!
        }

        std::string s(w.get_text());//Text to be written, if any

        //Format will depend on the word to be written
        switch(w.get_type())//Switch based on word type
        {
            //Start of text
            case WordType::START:
            {
                if (state==WordType::START)//If the stream is also at the start
                    ;//No need to do a thing
                else if (state==WordType::END)//If the stream is being reopened
                    ;//No need to do a thing
                else//Printing a start without the stream being closed/at the start, error
                    return false;

                break;
            }

            //End of text
            case WordType::END:
            {
                //Stream ended, print newline
                os<<'\n';

                break;
            }

            //Content
            case WordType::WORD:
            case WordType::SYMBOL:
            case WordType::INT:
            case WordType::DECIMAL:
            {
                switch(state)//Print based on previous word
                {
                    //Upper+Space
                    case WordType::R_STOP:
                    {
                        os<<' ';
                        if (!s.empty()&&std::islower(s[0]))
                            s[0]=std::toupper(s[0]);
                        break;
                    }

                    //Upper
                    case WordType::START:
                    case WordType::L_STOP:
                    {
                        if (!s.empty()&&std::islower(s[0]))
                            s[0]=std::toupper(s[0]);
                        break;
                    }

                    //Space
                    case WordType::WORD:
                    case WordType::SYMBOL:
                    case WordType::INT:
                    case WordType::DECIMAL:
                    case WordType::R_DELIM:
                    {
                        os<<' ';
                        break;
                    }

                    //Nothing (or unknown status)
                    case WordType::L_DELIM:
                    default:
                        break;
                }

                //Now that the word is ready, print it
                os<<s;

                break;
            }

            //Left delimiter
            case WordType::L_DELIM:
            case WordType::L_STOP:
            {
                switch (state)//Print based on previous word
                {
                    //Space

                    //After content
                    case WordType::WORD:
                    case WordType::SYMBOL:
                    case WordType::INT:
                    case WordType::DECIMAL:
                    //After right delimiter
                    case WordType::R_DELIM:
                    case WordType::R_STOP:
                    {
                        os<<' ';
                        break;
                    }

                    //Nothing

                    //Other left delimiters
                    case WordType::L_DELIM:
                    case WordType::L_STOP:
                    //Unknown
                    default:
                        break;
                }

                //Now that the word is ready, print it
                os<<s;

                break;
            }

            //Right delimiter
            case WordType::R_DELIM:
            case WordType::R_STOP:
            {
                //No formatting needed, just print it
                os<<s;
                break;
            }

            //By default
            default:
            {
                os<<s;//Just print the word
                break;
            }
        }

        //Update the stream's status
        state=w.get_type();

        //No error, return true
        return true;
    }

    /*
        WordModel
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Default constructor
    WordModel::WordModel()
    :graph()
    {}

    /* Methods */

    /*Learn*/

    //Learn from a text stream
    void WordModel::learn(ITextStream &ts)
    {
        //Load the first word
        if (ts.has_words())
        {
            Word w=ts.read();//Word to be processed
            graph.add_word(w);//Add it to the graph

            //Read the rest on a loop
            Word prev=w;//Previous word to be processed

            //While the stream has words
            while (ts.has_words())
            {
                //Read the word
                w=ts.read();

                //Add the word
                graph.add_word(w);

                //Add the links
                graph.add_link(prev,w);

                //Store the current word on previous
                prev=w;
            }
        }
    }

    /*Speak*/

    //Generate a line using the model
    void WordModel::think(OTextStream &ots)
    {
        //Make sure the start and end node exist

        if (!graph.check_word(Word(WordType::START)))
            graph.add_word(Word(WordType::START));

        if (!graph.check_word(Word(WordType::END)))
            graph.add_word(Word(WordType::END));

        WordNode *node=graph.get_node(Word(WordType::START));//The first node to be processed is the start node

        const Word end_word(WordType::END);


        while(node&&node->get_word()!=end_word)//Until the end node is reached
        {
            ots.write(node->get_word());//Print this node

            //Advance to next
            node=graph.get_node(node->get_next());
        }

        //Close the stream
        ots.write(end_word);
    }

    /*Similarity*/

    //Similarity between two nodes. Always zero if either word is not found
    prob_frec WordModel::similarity_word(const Word &w1, const Word &w2) const
    {
        return graph.similarity_word(w1,w2);
    }

    //Generate the clusters
    void WordModel::clustering()
    {
        //First, add the initial clusters
        for (const auto &word : graph.nodes)
        {
            clusters.emplace_back(word.first);
        }

        //Then, do the hierarchical clustering
        while(clusters.size()>1)//Continue joining until there's only one cluster left
        {
            prob_frec best_simil = 0;//Similarity of the most similar pair of clusters to join
            std::list<ClusterWord>::iterator best_c1=clusters.end(),best_c2=clusters.end();//Best clusters to join

            for (auto it1 = clusters.begin(); it1!=clusters.end(); ++it1)
            {
                for (auto it2 = std::next(it1); it2!=clusters.end(); ++it2)
                {
                    prob_frec simil = ClusterWord::similarity_cluster(*it1,*it2,graph);//Calculate the similarity between this pair of nodes

                    if (simil>best_simil)
                    {
                        best_c1=it1;
                        best_c2=it2;

                        best_simil=simil;
                    }
                }
            }

            if (best_c1!=clusters.end() && best_c2!=clusters.end())
            {
                if (best_c1->words.size() < best_c2->words.size())
                {
                    auto temp=best_c1;
                    best_c1=best_c2;
                    best_c2=temp;
                }

                best_c1->join_cluster(*best_c2);
                clusters.erase(best_c2);
            }
        }
    }

    /*Read/write to file*/

    //Write to file
    void WordModel::write(std::ostream &o) const
    {
        graph.write(o);
    }

    //Read from file
    void WordModel::read(std::istream &i)
    {
        graph.read(i);
    }

    /*
        ClusterWord
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Default constructor
    ClusterWord::ClusterWord()
    :words(),id(ClusterWord::get_id())
    {}

    //Constructor with initial word
    ClusterWord::ClusterWord(const Word &w)
    :words({w}),id(ClusterWord::get_id())
    {}

    /* Methods */

    /*Clustering*/

    //Similarity

    //Similarity between two clusters
    prob_frec ClusterWord::similarity_cluster(const ClusterWord &c1, const ClusterWord &c2, const WordGraph &g)
    {
        //Check if the value has already been calculated
        std::pair<cuid,cuid> key(std::min(c1.id,c2.id),std::max(c1.id,c2.id));

        auto it = ClusterWord::cache_simil.find(key);
        if (it!=ClusterWord::cache_simil.end())//If found, return it
            return it->second;


        prob_frec ponderated_sum=0;//Ponderated sum of the similarities (dividend)
        prob_frec weight_sum=0;//Sum of weights (divider)

        //Process the pair of words
        for (const Word &w1 : c1.words)//Iterate over the words of the first cluster
        {
            for (const Word &w2 : c2.words)//Iterate over the words of the second cluster
            {
                //Get the nodes of both words
                const WordNode *ptr_w1 = g.get_node(w1);
                const WordNode *ptr_w2 = g.get_node(w2);

                int f1 = ptr_w1==nullptr?0:ptr_w1->f;
                int f2 = ptr_w2==nullptr?0:ptr_w2->f;

                if (f1||f2)//Check that either of the words exists, before proceding
                {
                    prob_frec weight = std::hypot(f1,f2);//Weight of this value
                    prob_frec simil = g.similarity_word(w1,w2);

                    //Update the sums
                    ponderated_sum+=weight*simil;
                    weight_sum+=weight;
                }
            }
        }

        prob_frec rv=weight_sum?ponderated_sum/weight_sum:0;//Value to be returned

        //Save to cache before returning it
        ClusterWord::cache_simil.insert(std::map<std::pair<cuid,cuid>,prob_frec>::value_type(key,rv));

        return rv;
    }

    //Join a cluster into this one
    void ClusterWord::join_cluster(const ClusterWord &c)
    {
        auto pos = words.cbegin();
        for (auto it = c.words.cbegin(); it != c.words.cend(); ++it)
        {
            pos = words.insert(pos, *it);
        }

        id=ClusterWord::get_id();
    }

    /*IDs and caching*/

    //Get a valid ID
    ClusterWord::cuid ClusterWord::get_id()
    {
        return ClusterWord::guid++;
    }

}//End of namespace

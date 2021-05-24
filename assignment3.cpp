#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm> //for sort();
#include <sstream> //for stringstream
#include <unistd.h>

/*Compile with with:
*			g++ -std=gnu++11 assignment3.cpp
*Run with:
*			./a.out testSet.txt trainingSet.txt preprocessed_test.txt preprocessed_train.txt results.txt

************* Helper Function Sources***************
* - C Standard Library
*  https://www.cplusplus.com/reference/fstream/
*  https://www.cplusplus.com/reference/iostream/
*  https://www.cplusplus.com/reference/sstream/
*  https://www.cplusplus.com/reference/vector/
*  https://www.cplusplus.com/reference/algorithm/
*  https://www.cplusplus.com/reference/string/
*/

struct Word {
	std::string word = "";
	int positiveClassLabel;
	int negativeClassLabel;
	double probabilityNegative;
	double probabilityPositive;
};

//Function declarations
std::vector<Word> preprocessing(std::ifstream& trainingFIN, std::ofstream& trainingFOUT, int& numLines);
void push_to_vector(std::ifstream& trainingFIN, int& numLines, std::vector<Word>& vocabulary, std::vector<std::string>& featurized);
void make_unique(std::vector<Word>& vocabulary, std::vector<std::string>& featurized, std::vector<Word>& uniqueVocabulary, int& numLines, std::ifstream& trainingFIN, std::ofstream& trainingFOUT);
void removeDupes(std::vector<Word>& uniqueVocabulary);
void output_features(std::ofstream& trainingFOUT, std::vector<Word>& vocabulary, std::vector<std::string>& featurized, std::vector<Word>& uniqueVocabulary, int& numLines);
void validateOpenOfstream(std::ofstream& ofstreamVariable);
void validateOpenIfstream(std::ifstream& ifstreamVariable);
void dirichletPrior(std::vector<Word>& vocabulary);
void reset(std::ifstream& trainingFIN);
void classification_vector(std::ifstream& input, int& numLines, std::vector<Word>& vocabulary);
void classification(std::ifstream& input, std::ofstream& output, std::vector<Word>& vocabulary, int& numLines);
void getReviewCount(std::vector<double>& reviewActual, std::vector<double> reviewPrediction, std::vector<std::string> classificationVocabulary);

int main() {
	//create two ifstream objects to read from the test and training set text files.
	std::ifstream trainingFIN("trainingSet.txt");
	std::ifstream testFIN("testSet.txt");
	//next create the output objects for file output
	//per assignment constraints these are called preprocessed_test.txt and preprocessed_text.txt
	std::ofstream trainingFOUT("preprocessed_train.txt");
	std::ofstream testFOUT("preprocessed_test.txt");
	std::ofstream results("results.txt");
	//the final vocabulary list should be in alphabetical order.
	std::vector<Word> trainingVocabulary; //this will be the set of all words contained in the training data, obtained via preprocessing.
	std::vector<Word> testVocabulary;
	//preprocessing will return a vector of Words (a struct) to the vector<Word> vocabulary.
	int numLines = 0;

	/*************************************** PREPROCESSING *********************************/
	trainingVocabulary = preprocessing(trainingFIN, trainingFOUT, numLines); //pass the training set read and write to the preprocessing function.
	dirichletPrior(trainingVocabulary);
	numLines = 0;
	testVocabulary = preprocessing(testFIN, testFOUT, numLines);
	dirichletPrior(testVocabulary);

	sleep(5);
	reset(trainingFIN);
	reset(testFIN);

	/*************************************** CLASSIFICATION *********************************/
	//classification involves testing the accuracy of the preprocessed training set against the training set and the test set. 
	numLines = 0;
	//first we will test the accuracy against the training set, should be 90%+ accurate, sanity check
	std::cout << "Classification results for trainingSet.txT" << std::endl;
	classification(trainingFIN, results, trainingVocabulary, numLines);

	std::cout << "Classification results for testSet.txT" << std::endl;
	numLines = 0;
	classification(testFIN, results, trainingVocabulary, numLines);

	//then we will test the accuracy against the test set.

}

void classification_vector(std::ifstream& input, int& numLines, std::vector<Word>& vocabulary) {
	std::string currentLine;
	char spacer = ' ';
	std::string currentWord = "";
	std::vector<std::string> classificationVocabulary;
	std::vector<double> wordNegative;
	std::vector<double> wordPositive;
	std::vector<double> predictedReview;
	std::vector<double> reviewActual;
	double negativeProbabilityCurrent = 1.;
	double positiveProbabilityCurrent = 1.;
	/*
	for (int i = 0; i < vocabulary.size(); i++) {
		std::cout << "Word: " << vocabulary[i].word<<std::endl;
		std::cout << "Positive Probability: " << vocabulary[i].probabilityNegative << std::endl;
		std::cout << "Negative Probability: " << vocabulary[i].probabilityPositive << std::endl;
	}*/

	/*
	for (int i = 0; i < vocabulary.size(); i++) {
		std::cout << vocabulary[i].word << std::endl;
		std::cout << vocabulary[i].positiveClassLabel << std::endl;
		std::cout << vocabulary[i].negativeClassLabel << std::endl;
		std::cout << "Probability Negative: " << vocabulary[i].probabilityNegative << " Probability Positive: " << vocabulary[i].probabilityPositive << std::endl;
	}
	sleep(1);
	*/

	while (!input.eof()) { //while we are not at the end of the file
		getline(input, currentLine); //get the current line
		if (!currentLine.empty()) {
			numLines++; //increment the number of lines
			char classLabel = currentLine[currentLine.length() - 2]; //store the class label
			for (int i = 0; i < currentLine.size(); i++) { //remove punctuation and upper case
				if (ispunct(currentLine[i])) { //if there is punctuation then erase it
					currentLine.erase(i--, 1);
				}
				if (isupper(currentLine[i])) { //if there is upper case, make it lower case
					currentLine[i] = tolower(currentLine[i]);
				}
			}

			for (int i = 0; i < currentLine.size(); i++) { //find spaces and push words into the classification vocabulary
				if ((char)currentLine[i] == spacer) { //space check
					classificationVocabulary.push_back({ currentWord }); //push
					for (int j = 0; j < vocabulary.size(); j++) { //if we pushed a word onto the set, we need to compare against our training vocabulary.
						if (classificationVocabulary.back() == "\t") { //if we find a tab space then we have reached the end of the sentence.
							for (int x = 0; x < wordNegative.size(); x++) { //multiply through the thing
								if (wordNegative[x] != 0.) { //if not zero or else our probabiliy will be fucked
									negativeProbabilityCurrent = wordNegative[x] * negativeProbabilityCurrent;
								}
							}
							for (int y = 0; y < wordPositive.size(); y++) { //multiply through the thing
								if (wordPositive[y] != 0.) {
									positiveProbabilityCurrent = wordPositive[y] * positiveProbabilityCurrent;
								}
							}
							if (positiveProbabilityCurrent > negativeProbabilityCurrent) {
								predictedReview.push_back(1);
							}
							else if(negativeProbabilityCurrent > positiveProbabilityCurrent) {
								predictedReview.push_back(0);
							}
					
							wordPositive.clear();
							wordNegative.clear();
							negativeProbabilityCurrent = 1.;
							positiveProbabilityCurrent = 1.;
						}
						else if (vocabulary[j].word == classificationVocabulary.back()) { //if it is not a tab, then we see if it is a word and push its probabilities onto the vectors if it is.
							wordNegative.push_back(vocabulary[j].probabilityNegative);
							wordPositive.push_back(vocabulary[j].probabilityPositive);

						}
					}
					currentWord = "";
				}
				else { //if the character is not a space then add it to currentWord.
					currentWord = currentWord + currentLine[i];
				}
				
			}
		}
		else {
			//do nothing
		}
	}

	getReviewCount(reviewActual, predictedReview, classificationVocabulary);
}




	//(reviewActual, reviewPredictions, classificationVocabulary);

void getReviewCount(std::vector<double>& reviewActual, std::vector<double> reviewPrediction, std::vector<std::string> classificationVocabulary) {
	int numPositive = 0;
	int numNegative = 0;
	int positiveCorrect = 0;
	int negativeCorrect = 0;
	std::cout << classificationVocabulary.size() << std::endl;
	for (int i = 0; i < classificationVocabulary.size(); i++) {
		if (classificationVocabulary[i] == "\t") {
			if (i + 1 < classificationVocabulary.size()) {
				if (classificationVocabulary[i + 1] == "1") {
					numPositive++;
					reviewActual.push_back(1);
				}
				else if (classificationVocabulary[i + 1] == "0") {
					numNegative++;
					reviewActual.push_back(0);
				}
			}
		}
	}

	for (int j = 0; j < reviewPrediction.size(); j++) {
		if ((reviewActual[j] == 1) && (reviewPrediction[j] == 1)) {
			positiveCorrect++;
		}
		else {
			negativeCorrect++;
		}
	}
	double accuracyPositive = (double)positiveCorrect / (double)numPositive;
	double accuracyNegative = (double)negativeCorrect / (double)numNegative;
	double totalAccuracy = ((double)accuracyPositive + (double)accuracyPositive) / 2.;

	std::cout << "Predicted Positive: " << positiveCorrect << " Predicted Negative: " << negativeCorrect << std::endl;
	std::cout << "Actual Positive: " << numPositive << " Actual Negative: " << numNegative << std::endl;
	std::cout << "Positive Accuracy: "<< accuracyPositive << std::endl;
	std::cout << "Negative Accuracy: " << accuracyNegative << std::endl;
	std::cout << "Total accuracy: " << totalAccuracy << std::endl;
}

void classification(std::ifstream& input, std::ofstream& output, std::vector<Word>& vocabulary, int &numLines) {
	validateOpenIfstream(input);
	validateOpenOfstream(output);
	classification_vector(input, numLines, vocabulary);
}


/**********************************************
* Function: sortVocabulary
* Description: Simple function that reset the eof flag bit and returns it to the beginning, just to clean up the main() function.
* Parameter: stream variables
**********************************************/

void reset(std::ifstream& trainingFIN) {
	trainingFIN.clear(); //ios::clear resets the flags of the ifstream variable. Particularly, we read to the end of file and set the eof flag to true.
	//running clear will set the eof flag to false again
	trainingFIN.seekg(0, std::ios::beg); //in this case, we want the end trainingFIN (file input stream) to be set to the beginning using seekg().
} 
	
/**********************************************
* Function: dirichletPrior
* Description: Fill this
* Parameter: the Word vector
**********************************************/

void dirichletPrior(std::vector<Word>& vocabulary) {
	//dirichlet priors formula
	//total number of records 
	//each value that doesnt exist is equally likely int he absence of data.  This 1/Nj value is dominated by values that do have existing probabilities.
	for (int i = 0; i < vocabulary.size(); i++) {
		double denominator = ((double)(vocabulary[i].positiveClassLabel + (double)vocabulary[i].negativeClassLabel) + 2.);
		//in the denominator, 2 is the number of values that the probability can take on. (positive or negative)
		//For each word in the vocabulary, calculate the probability that a word came from the positive or the negative by adding its class label values.
		vocabulary[i].probabilityNegative = ((double)vocabulary[i].negativeClassLabel + 1.) / denominator;
		vocabulary[i].probabilityPositive = ((double)vocabulary[i].positiveClassLabel + 1.) / denominator;
	}
}

/**********************************************
* Function: sortVocabulary
* Description: Takes in two word objects and compares their .text member variables. If A is less than B then return true, else return false.
*			   helper function that is called as the third argument of the standard library C++ sort function.
* Parameter: two word objects
* Output: True or false
**********************************************/
bool sortVocabulary(const Word& a, const Word& b) {
	return a.word < b.word; 
}

/**********************************************
* Function: sortVocabulary
* Description: Takes in two word objects and compares their .text member variables. If A is less than B then return true, else return false.
*			   helper function that is called as the third argument of the standard library C++ sort function.
* Parameter: two word objects
* Output: True or false
**********************************************/
bool checkSentence(std::string sentence, std::string word) {
	//we create a stringstream class object called s which
	std::stringstream s(sentence);
	std::string temp;
	while (s >> temp) { //use the bitshifting operator to move rightward through the sentence and store its contents in temp
		if (temp.compare(word) == 0) { //the compare function returns 0 if there is a word that matches temp, and if so return true
			return true; 
		}
	}
	return false;
}

/**********************************************
* Function: validateOpen
* Description: For bug fixing. Just checks if the file is open and if not then we know the file is invalid, just exit the program
* Parameter: ifstream trainingFIN pass by address
**********************************************/

void validateOpenIfstream(std::ifstream& ifstreamVariable) {
	if (ifstreamVariable.is_open()) {
		std::cout << "Ifstream variable was successfully opened." << std::endl;
	}
	else {
		std::cout << "Failed to open ifstream variable. Invalid - exiting program." << std::endl;
		exit(0);
	}
}

//same as above but for ofstream
void validateOpenOfstream(std::ofstream& ofstreamVariable) {
	if (ofstreamVariable.is_open()) {
		std::cout << "Ofstream variable was successfully opened." << std::endl;
	}
	else {
		std::cout << "Failed to open ofstream variable. Invalid - exiting program." << std::endl;
		exit(0);
	}
}

/**********************************************
* Function: preprocessing
* Description: outer layer function for preprocessing, makes calls to a bunch of functions that do stuff
*			   at the end, this function will print the contents of the vector to the preprocessing_training.txt
*			   and return the vocabulary to the main function.
* Param: ofstream, ifstream, numLines.
**********************************************/

std::vector<Word> preprocessing(std::ifstream &trainingFIN, std::ofstream &trainingFOUT, int &numLines) {
	validateOpenIfstream(trainingFIN); //if the file is open then we will remove punctuation and convert stuff to lower.
	std::vector<Word> vocabulary; //stores vocabulary
	std::vector<std::string> featurized; //stores the sentences
	std::vector<Word> uniqueVocabulary; //stores unique vocabulary
	//preprocess and push the appropriate values onto our vectors
	std::cout << "Please wait, the pre-processing stage may take a bit." << std::endl;
	push_to_vector(trainingFIN, numLines, vocabulary, featurized);
	make_unique(vocabulary, featurized, uniqueVocabulary, numLines, trainingFIN, trainingFOUT);
	sort(uniqueVocabulary.begin(), uniqueVocabulary.end(), sortVocabulary);
	validateOpenOfstream(trainingFOUT);

	/*************************************** OUTPUT TO FILE **********************************************/

	for (int i = 0; i < numLines + 1; i++) {
		if (i == 0) { //if the line number is 0, then we want to print all of the words int he vocabulary alphabetized on a single line.
			for (int j = 0; j < uniqueVocabulary.size(); j++) { //iterate through and output the entire vocabulary to the file on a single line
				trainingFOUT << uniqueVocabulary[j].word << ",";
			}
			trainingFOUT << "classlabel\n"; //dummy non-word at the end of the vocabulary + new line character.
		}
		else { //all lines that are not the first line
			for (int k = 0; k < uniqueVocabulary.size(); k++) {
				if (checkSentence(featurized[i - 1], uniqueVocabulary[k].word)) { //iterate through each sentence in the featurized list.
					// If the word at index k shows up in the sentence at index i-1 then print out a 1.
					trainingFOUT << "1,";
				}
				else { //if teh word at index k does not show up in the sentence at index i - 1 print a 0.
					trainingFOUT << "0,";
				}
			}
			trainingFOUT << featurized[i - 1].back() << "\n"; //newline at the end of each word.
		}
	}
	return uniqueVocabulary;
}



/**********************************************
* Function: make_unique
* Description: This function will take the vocabulary set and make it only have unique words by re-evaluating the positive and negative class labels after iterating through and
*				removing duplicate words. All vectors passed by reference.
* Param: vocab set, featurized string set, unique vocabulary set, numlines, filein and fileout objects.
**********************************************/

void make_unique(std::vector<Word>& vocabulary, std::vector<std::string>& featurized, std::vector<Word> &uniqueVocabulary, int& numLines, std::ifstream& trainingFIN, std::ofstream& trainingFOUT) {
	/***************** PUSH TO VOCABULARY SET AND FEATURIZED SET ***********************/

	int positiveLabelValue = 0;
	int negativeLabelValue = 0;
	std::sort(vocabulary.begin(), vocabulary.end(), sortVocabulary); //after we have finished processing everything we can sort.
	int j = 0;
	for (int i = 0; i < vocabulary.size(); i++) { //have an iterator go throuugh the vocabulary
		for (j = i + 1; j < vocabulary.size(); j++) { //have an iterator that starts 1 step ahead of i go thorugh the vocabulary
			if (vocabulary[i].word == vocabulary[j].word) { //if the vocabulary at i matches the vocabulary at j
				if (vocabulary[j].positiveClassLabel == 1) {  //increment the positive or negative label based on j
					positiveLabelValue++;
				}
				else {
					negativeLabelValue++;
				}
			}
			else if (vocabulary[i].word != vocabulary[j].word) {
				if (vocabulary[i].positiveClassLabel == 1) { //first check the class label of index i
					positiveLabelValue++;
				}
				else if (vocabulary[i].negativeClassLabel == 1) {
					negativeLabelValue++;
				}
				Word finalWord; //create a new temporary word
				//std::cout << "Vocabulary word: " << vocabulary[i].word << std::endl;
				finalWord.word = vocabulary[i].word; //set the word to the word
				//std::cout << "Final word: " << finalWord.word << std::endl;
				finalWord.positiveClassLabel = positiveLabelValue;
				finalWord.negativeClassLabel = negativeLabelValue; //assign its label values
				//std::cout << "Final word labels: " << finalWord.positiveClassLabel << " : " << finalWord.negativeClassLabel << std::endl;
				uniqueVocabulary.push_back(finalWord); //push the word onto uniqueVocabulary
				i = j; //skip the redundant iterations
				positiveLabelValue = 0;
				negativeLabelValue = 0; //reset the values
			}
		}
	}
	removeDupes(uniqueVocabulary);
}

/**********************************************
* Function: push_to_vector
* Description: Reads the input file and creates a cleaned up vector containing Words and a vector of strings to contain each sentence from the text file.
* Param: ifstream variable, numLines, two vectors: one of type Word and one of type string.
**********************************************/

void push_to_vector(std::ifstream& trainingFIN, int& numLines, std::vector<Word>& vocabulary, std::vector<std::string>& featurized) {
	std::string currentLine; //a string used in getline.
	Word currentWord; //for storing the current Word and its additional parameters, particularly whether its positive or negative at this point in time.
	std::string currentSentence = ""; //for storing the current sentence
	std::string currentText = ""; //for storing the current word
	char spacer = ' '; //character just for seeing if there are spaces later on
	
	while (!trainingFIN.eof()) {


		/***************** FOR EACH LINE IN THE TEXT FILE ***********************/


		getline(trainingFIN, currentLine);
		if (!currentLine.empty()) { //if the current line is not empty then we will process it
			numLines++; //increment number of lines by 1.
			//grab the digit before processing the line.
			char classLabel = currentLine[currentLine.length() - 3]; //grab the 0 or 1 that is at the end of every line in the .txt and store it.
			// Remove the weird spacings that surround both the tab character and the class labe.
			currentLine.erase((currentLine.length() - 2), 2); //for some reason in the text file there is
			currentLine.erase((currentLine.length() - 4), 1);


			/***************** CLEAN UP THE CURRENT LINE ***********************/


			for (int i = 0; i < currentLine.size(); i++) { //iterate through every index in the line and see if there is a punctuation, digit, tab or upper case.
				//ispunct will check for punctuation. \t will check for tab character, isdigit will check for any numbers which is inclusive of the review value.
				if (ispunct(currentLine[i]) || isdigit(currentLine[i]) || (char)currentLine[i] == '\t') {
					currentLine.erase(i--, 1);
				}
				if (isupper(currentLine[i])) {
					currentLine[i] = tolower(currentLine[i]);
				}
			}			

			/***************** PUSH TO VOCABULARY SET AND FEATURIZED SET ***********************/

			
			for (int i = 0; i < currentLine.length(); i++) {
				if ((char)currentLine[i] != spacer) { //if the current character is not space then add it to the current
					//we add the current character to the currentWord and currentSentence strings.
					currentText = currentText + (char)currentLine[i]; //append the index to the string.
					currentSentence = currentSentence + (char)currentLine[i];
				}
				else if((char)currentLine[i] == spacer) { //the character is a space then the word is finished and we can push to the vector and go next
					if (classLabel == '1') {
						currentWord.word = currentText; //push the word to the thing
						//push the word class label onto the currentWord in the context of the sentence
						currentWord.positiveClassLabel = 1; 
						currentWord.negativeClassLabel = 0;
					}
					else if(classLabel == '0') {
						currentWord.word = currentText;
						//push the word class label onto the currentWord in the context of the sentence
						currentWord.positiveClassLabel = 0; 
						currentWord.negativeClassLabel = 1;
					}

					currentText = ""; //reset the current word to nothing
					currentSentence = currentSentence + currentLine[i]; //add the space to the sentence and continue
					vocabulary.push_back(currentWord); //pushback the Word object into the vector of Words after we assign it currentText and the class label.
				}
			}
		
			//when we are done processing the current line we can push onto the vector for featurized data
			//featurized will be the vocabulary set + the class label value.
			currentSentence = currentSentence + " " + classLabel; //add a space followed by the class label to the end of the sentence

			featurized.push_back(currentSentence); //push it onto the vector.
			currentSentence = ""; //reset the sentence
			//the sort function sorts the elemennts  in the range from vocabulary begin to vocabulary end using a binary function that accepts two functions
			//and returns a boolean.


		}
		else { //IF THE LINE IS EMPTY
			//std::cout << "Empty Line" << std::endl;
		}
	} //end while loop
}

/**********************************************
* Function: removeDupes
* Description: Use double for loop O(n^2) to check for dupes. If there are dupes we use the standard vector functions to move duplicates to the back of the list and pop them off.
*				We only care about the first value located inside the unique vocabulary list because it has the exact positive and negative values found from the text file.
* Param: unique vocabulary vector
**********************************************/

void removeDupes(std::vector<Word>& uniqueVocabulary) {
	for (int i = 0; i < uniqueVocabulary.size(); i++) {
		for (int j = i + 1; j < uniqueVocabulary.size(); j++) {
			//we only want the first instance of each word. It will have the exact positive and negative values for the unique instance of a word.
			if(uniqueVocabulary[i].word == uniqueVocabulary[j].word) { //if a duplicate is found
				uniqueVocabulary[j].word = uniqueVocabulary[uniqueVocabulary.size() - 1].word; //move the value at [j] to the end of the vocabulary list and pop_back.
				uniqueVocabulary.pop_back(); //pops it off of the vector, pop_back removes the last value in our list (the back value) :)
				j--; // decrement afterwards and continue. (We do this because we don't want to skip the new index at j.
			}
		}
	}
}



//template

/**********************************************
* Function:
* Description:
* Param:
**********************************************/
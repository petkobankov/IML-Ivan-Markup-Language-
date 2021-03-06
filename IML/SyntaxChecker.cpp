#include "SyntaxChecker.h"
#include <fstream>
#include <iostream>
#include "MyException.h"
using namespace std;
void SyntaxChecker::defaultState()
{
	if (currentInput.isSpace()) {
		//By default if the input is space it's ignored. The state is final, so that if the input stops there is no syntax error.
		updateStateFromTo(main_state::none, main_state::none,true);
	}
	else if (currentInput.isDigit() || currentInput.isDash()) {
		//if the current input is -, or a digit then we move to the number state. The number state has sub states. 
		updateStateFromTo(main_state::none, main_state::number);
		//The stream is paused so that the code is more understandable. 
		pauseStreamPointer();
	}
	else if (currentInput.isLeftArrow()) {
		//The input is '<' so the program prepares to read a tag.
		updateStateFromTo(main_state::none, main_state::tag);
		pauseStreamPointer();
	}
	else {
		//When throwing an exception, this message is added for additional information. 
		error_msg = "Excepting only numbers between tags.";
		if (currentInput.isRightArrow())
			error_msg = "Tags must start with '<'.";
		//If the input is anything else from space,digit,dash,left arrow the program outputs there is error with the syntax. 
		updateStateFromTo(main_state::none, main_state::error);
	}
}

void SyntaxChecker::tagState()
{
	//Like in checkSyntax() here the programm goes through sub states of a tag. 
	switch (currentTagState)
	{
	case 's':
		//The start of automata, expects '<' else it's error. If we want spaces between '<' and tag name, it should be added here.
		if (currentInput.isLeftArrow()) {
			updateTagState('z');
		}
		else {
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'z':
		//This states seperates a start tag from ending one. The ending one has no additional inputs.
		if (currentInput.isUpperCase()) {
			updateTagState('n');
		}
		else if (currentInput.isSlash()) {
			updateTagState('e');
		}
		else {
			error_msg = "Tag name can't start with other than upper case letter.";
			if (currentInput.isLowerCase())
				error_msg = "Excepting only upper case letters for tag name.";
			if (currentInput.isRightArrow())
				error_msg = "Tags can't be empty.";
			if (currentInput.isLeftArrow())
				error_msg = "Only one left arrow is needed.";				
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'n':
		//The state for a starting tag. Currently reading the name. 
		//If there is a space, then we expect additional input, if there is '>' then that's the end of the tag and the program changes the state to default in exitFromTagState();
		//If we want to accept space between TAG-NAME and > we make the change here.
		if (currentInput.isUpperCase() || currentInput.isDash()) {
			updateTagState('n');
		}
		else if (currentInput.isSpace()) {
			updateTagState('a');
		}
		else if (currentInput.isRightArrow()) {
			exitFromTagState();
		}
		else {
			error_msg = "Tag name should be in upper case letters(A '-' is allowed in the name).";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'a':
		//Expecting a quote for additional input
		if (currentInput.isQuote()) {
			updateTagState('d');
		}
		else {
			error_msg = "Expected a '\"'. There should be one col. difference between tag name and additional info.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'd':
		//Seperates the states to a tag with addition string input, and a tag with additional float/int input.
		if (currentInput.isUpperCase()) {
			updateTagState('c');
		}
		else if (currentInput.isDash() || currentInput.isDigit()) {
			//If it's a digit or - we change the main state to 'number' because the program is reading a number
			//We update the prev state to 'tag' so that if the input stops in a final state the syntax is still incorrect
			updateTagState('b');
			updateStateFromTo(main_state::tag, main_state::number);
			pauseStreamPointer();
		}
		else {
			error_msg = "Additional info for a tag can only be a string(upper case letters) or a number.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'c':
		//Expecting string with upper case letters, if there is a quote then the additional input is over
		if (currentInput.isUpperCase()) {
			updateTagState('c');
		}
		else if (currentInput.isQuote()) {
			updateTagState('t');
		}
		else {
			error_msg = "Additional info for tag should be in upper case letters if it's a string.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'b':
		//When the main state is number, it reads digits until the input is something else. When the input is other than a number it goes back to it's previous state i.e. 'tag' in our case. The sub state was changed to b when we updated the main state so we are here.
		//Expecting only '"'
		if (currentInput.isQuote()) {
			updateTagState('t');
		}
		else {
			error_msg = "Additional info for tag should consist of digits, '-' or '.' if it's a number.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 't':
		//After the quote there is nothing else to read expect '>'. If we want to read extra spaces between the two, we change it here.
		if (currentInput.isRightArrow()) {
			exitFromTagState();
		}
		else {
			error_msg = "A '>' was expected. The syntax doesn't allow spaces between end tag and internals.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	case 'e':
		//This state was from the start of the function, when we read '/'. So it's an end tag. 
		//For an end tag we expect only letters, dashes or '>'
		//If it's '>' then we exit from the tag state
		if (currentInput.isUpperCase()|| currentInput.isDash()) {
			updateTagState('e');
		}
		else if (currentInput.isRightArrow()) {
			exitFromTagState();
		}
		else {
			error_msg = "The syntax for end tag is </TAG-NAME>. No spaces allowed.";
			updateStateFromTo(main_state::tag, main_state::error);
		}
		break;
	default:
		break;
	}
}

void SyntaxChecker::numberState()
{
	//Like tagState().
	switch (currentNumState) {
	case 's':
		//In the start we expect either - or a digit
		if (currentInput.isDash()) {
			updateNumState('m');
		}
		else if (currentInput.isDigit()) {
			updateNumState('n',true);
		}
		else {
			updateStateFromTo(main_state::number, main_state::error);
		}
		break;
	case 'm':
		//If we read a - then we expect atleast one digit after it.
		if (currentInput.isDigit()) {
			updateNumState('n',true);
		}
		else {
			error_msg = "There should be at least one digit after a '-'.";
			updateStateFromTo(main_state::number, main_state::error);
		}
		break;
	case 'n':
		//We have read at least one digit here. If we read something else than a digit we exit from main state 'number', go back to the previous main state and pause the stream for one cycle
		if (currentInput.isDigit()) {
			updateNumState('n',true);
		}
		else if (currentInput.isDot()) {
			updateNumState('d');
		}
		else {
			exitFromNumState();
		}
		break;
	case 'd':
		//We have read one dot, now it's not a final state and we expect atleast one more digit.
		if (currentInput.isDigit()) {
			updateNumState('a',true);
		}
		else {
			error_msg = "There should be at least one digit after a dot.";
			updateStateFromTo(main_state::number, main_state::error);
		}
		break;
	case 'a':
		//We have read at least one digit after a dot, it's a final state so if we read something else from a digit we change the main state to the previous one and pause the stream for a cycle.
		if (currentInput.isDigit()) {
			updateNumState('a', true);
		}
		else {
			exitFromNumState();
		}
		break;
	default:
		break;
	}
}

void SyntaxChecker::errorState()
{
	//Called when the main state is 'error'.
	//Throws expection and additonal message.
	stopCheck();
}

void SyntaxChecker::updateStateFromTo(main_state from, main_state to, bool isNewStateFinal)
{
	//Called when we want to change the main state
	currentMainState = to; 
	prevMainState = from;
	//We mark the state final (or not final) before we are in it, because if there is no more input we wouldn't know if the last state was final or not.
	lastStateWasFinal = isNewStateFinal;
}

void SyntaxChecker::updateTagState(const char& newTagState, bool isFinal)
{
	//Same as updateStateFromTo
	currentTagState = newTagState;
	lastStateWasFinal = isFinal;
}

void SyntaxChecker::updateNumState(const char& newNumState, bool isFinal)
{
	//Same as updateStateFromTo
	//If the previous state was a tag, we don't care if the last state is final, because we are currently still in a tag.
	currentNumState = newNumState;
	if (prevMainState == main_state::tag) {
		lastStateWasFinal = false;
	}
	else {
		lastStateWasFinal = isFinal;
	}
	
}

void SyntaxChecker::exitFromTagState()
{
	//For the next tag we make sure it starts from the beggining.
	updateTagState('s');
	//It's a final state, because if there is no more input there isin't an error.
	updateStateFromTo(main_state::tag, main_state::none,true);
}

void SyntaxChecker::exitFromNumState()
{
	//Same as exitFromTagState();
	updateNumState('s');
	//As in updateNumState, if the previous state was a tag, then the syntax is still currently incorrect.
	if (prevMainState != main_state::tag) {
		updateStateFromTo(main_state::number, prevMainState);
	}
	else {
		updateStateFromTo(main_state::number, prevMainState,true);
	}
	pauseStreamPointer();
}

void SyntaxChecker::pauseStreamPointer()
{
	iFile.seekg(-1, ios::cur);
	updateLineCount(false);
}

void SyntaxChecker::stopCheck()
{
	iFile.close();
	if (!canFinish()) throw MyException(e_type::syntax, error_msg, line_count, col_count-1);
}

void SyntaxChecker::updateLineCount(bool positive)
{
	int inc;
	positive ? inc = 1 : inc = -1;
	if (currentInput.isNewLine()) {
		line_count+=inc;
		col_count = 1;
	}
	else {
		col_count += inc;
	}
}

void SyntaxChecker::checkSyntax()
{
	if (inputFile.empty()) {
		throw MyException(e_type::error, "Input file missing!");
	}
	iFile.open(inputFile);

	//Main loop. Here we only operate on the main state and we are currently ignoring the sub states. 
	while (iFile.get(currentInput.getInput())) {
		switch (currentMainState) {
		case main_state::none:
			defaultState();
			break;
		case main_state::tag:
			tagState();
			break;
		case main_state::number:
			numberState();
			break;
		case main_state::error:
			errorState();
			break;
		default:
			throw MyException(e_type::internal);
			break;
		}
		updateLineCount();
	}
	stopCheck();
	//If the code reaches here then the syntax is correct.
}

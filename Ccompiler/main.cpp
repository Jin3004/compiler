#include <iostream>
#include <string>
#include <array>
#include <optional>
#include <vector>
#include <memory>

#define Debug(var) std::cout << var


//ƒNƒ‰ƒX‚Æ‚©‚ÌéŒ¾
template<class T>
using Ptr = std::shared_ptr<T>;

enum class TOKENTYPE {
	NUMBER,
	CHARACTER,
	STRING,
	SYMBOL,
	KEYWORD,
	IDENTIFIER,
	NONE
};

enum class NODETYPE {
	ADD,
	SUB,
	MUL,
	DIV,
	NUMBER,
	NONE
};

enum class DEBUGTYPE {
	ERROR
};

class Token {
public:
	TOKENTYPE type;
	std::string string;

	Token() {
		type = TOKENTYPE::NONE;
		string = "";
	}
};

class Node {
public:
	NODETYPE type;
	Ptr<Node> rhs;
	Ptr<Node> lhs;
	std::optional<int> num; //—tƒm[ƒh‚Ìê‡‚É‚Í”š‚ğ‚Â

	Node() {
		type = NODETYPE::NONE;
		rhs = nullptr;
		lhs = nullptr;
		num = std::nullopt;
	}

};


//ƒOƒ[ƒoƒ‹•Ï”
std::string src = "10 + 20 + 30";
std::vector<std::string> symbols = { "(", ")", "{", "}", ";", "=", "[", "]", ",", "+", "-", "*", "/", ".", "+=" };
std::vector<std::string> keywords = { "int", "for" };
std::array<char, 3> white_space = { ' ', '\n', '\t' };
std::vector<Token> tokens = {};
std::shared_ptr<Node> root = std::make_shared<Node>();


namespace Utility {

	template<typename T = char, typename U = std::vector<char>>
	bool fullSearch(T obj, U& container) {
		int size = std::size(container);
		for (int i = 0; i < size; ++i) {
			if (obj == container[i])return true;
		}
		return false;
	}

	void debug(DEBUGTYPE state, const char* msg) {
		if (state == DEBUGTYPE::ERROR)std::cout << "[ERROR]: " << msg << '\n';
	}

}

void Tokenize() {

	//int transition = 0;
	TOKENTYPE transition = TOKENTYPE::NONE;
	TOKENTYPE prev_transition = transition;
	int size = src.length();
	src += ' ';
	std::string tmp_string = "";

	auto MakeToken = [&]() {
		Token tmp;
		tmp.string = tmp_string;
		tmp.type = transition;
		tokens.push_back(tmp);
		tmp_string = "";
	};

	for (int pos = 0; pos < size; ++pos) {

		char cur = src[pos];
		char next = src[pos + 1];
		prev_transition = transition;

		if (transition == TOKENTYPE::NONE) {

			if (Utility::fullSearch(cur, white_space))continue;

			if ('0' <= cur && cur <= '9')transition = TOKENTYPE::NUMBER;
			else if (cur == '\'')transition = TOKENTYPE::CHARACTER;
			else if (cur == '\"')transition = TOKENTYPE::STRING;
			else if (('a' <= cur && cur <= 'z') || ('A' <= cur && cur <= 'Z') || (cur == '_'))transition = TOKENTYPE::IDENTIFIER;
			else  if (Utility::fullSearch(std::string{} +cur, symbols)) {
				transition = TOKENTYPE::SYMBOL;
			}

		}

		tmp_string += cur;

		bool will_make_token = false;

		if (transition == TOKENTYPE::NUMBER)if (next < '0' || '9' < next)will_make_token = true;
		if (prev_transition == TOKENTYPE::CHARACTER && transition == TOKENTYPE::CHARACTER)if (cur == '\'')will_make_token = true;
		if (prev_transition == TOKENTYPE::STRING && transition == TOKENTYPE::STRING)if (cur == '\"')will_make_token = true;
		if (transition == TOKENTYPE::IDENTIFIER) {
			bool whether_to_continue = ('a' <= next && next <= 'z') || ('A' <= next && next <= 'Z') || (next == '_') || ('0' <= next && next <= '9');
			if (!whether_to_continue)will_make_token = true;
		}
		if (transition == TOKENTYPE::SYMBOL) {
			if (!Utility::fullSearch(tmp_string + next, symbols))will_make_token = true;
		}

		if (will_make_token) {
			MakeToken();
			transition = TOKENTYPE::NONE;
			continue;
		}

	}

}

void Parse() {

	auto MakeNode = [](NODETYPE type, Ptr<Node> lhs, Ptr<Node> rhs)->Ptr<Node> {	
		Ptr<Node> node{};
		node->type = type;
		node->lhs = lhs;
		node->rhs = rhs;
		return node;
	};

	auto MakeLeaf = [](int num)->Ptr<Node> {
		Ptr<Node> node{};
		node->num = num;
		node->type = NODETYPE::NUMBER;
		return node;
	};



}

int main() {

	Tokenize();

	auto token_size = tokens.size();
	for (int i = 0; i < token_size; ++i) {
		switch (tokens[i].type) {
		case TOKENTYPE::CHARACTER:
			std::cout << "CHARACTER: ";
			break;
		case TOKENTYPE::IDENTIFIER:
			std::cout << "IDENTIFIER: ";
			break;
		case TOKENTYPE::KEYWORD:
			std::cout << "KEYWORD: ";
			break;
		case TOKENTYPE::NONE:
			std::cout << "NONE: ";
			break;
		case TOKENTYPE::NUMBER:
			std::cout << "NUMBER: ";
			break;
		case TOKENTYPE::STRING:
			std::cout << "STRING: ";
			break;
		case TOKENTYPE::SYMBOL:
			std::cout << "SYMBOL: ";
			break;
		}
		std::cout << tokens[i].string << '\n';
	}

}


/*

y”õ–Y˜^z(–Y‚ê‚È‚¢‚½‚ß‚É)

*/
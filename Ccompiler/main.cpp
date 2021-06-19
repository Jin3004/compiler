#include <iostream>
#include <string>
#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <functional>
#include <cassert>
#include <fstream>
#include "magic_enum/include/magic_enum.hpp"

#define Debug(var) std::cout << var << "\n"

//クラスとかの宣言
template<class T>
using Ptr = std::shared_ptr<T>;
using namespace magic_enum;

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
	ADD, // +
	SUB,// -
	MUL,// *
	DIV,// /
	LESS,// <
	LESS_OR_EQUAL,// <=
	EQUAL,// ==
	NOT_EQUAL,// !=
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
	std::optional<int> num; //葉ノードの場合には数字を持つ

	Node() {
		type = NODETYPE::NONE;
		rhs = nullptr;
		lhs = nullptr;
		num = std::nullopt;
	}

};


//グローバル変数
std::string src = "2 * 3 + 4 * 5";
std::vector<std::string> symbols = { "(", ")", "+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">=", "!" };
std::vector<std::string> keywords = { "int", "for" };
std::array<char, 3> white_space = { ' ', '\n', '\t' };
std::vector<Token> tokens{};
std::shared_ptr<Node> root = std::make_shared<Node>();
std::string assembly_code = "";


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

void TokenizeTest() {

	int token_size = tokens.size();

	for (int i = 0; i < token_size; ++i) {
		std::cout << enum_name(tokens[i].type) << ": " << tokens[i].string << "\n";
	}

}

void Parse() {

	int pos = 0; //今何番目のトークンを参照しているのか。
	auto Consume = [&](std::string str)->bool {
		if (pos >= (int)tokens.size())return false;
		if (tokens[pos].string == str) {
			++pos;
			return true;
		}
		return false;
	};

	auto MakeNode = [](NODETYPE type, Ptr<Node> lhs, Ptr<Node> rhs)->Ptr<Node> {
		Ptr<Node> node = std::make_shared<Node>();
		node->type = type;
		node->lhs = lhs;
		node->rhs = rhs;
		return node;
	};

	auto MakeNum = [](int num)->Ptr<Node> {
		Ptr<Node> node = std::make_shared<Node>();
		node->num = num;
		node->type = NODETYPE::NUMBER;
		return node;
	};


	//以下はそれぞれ前方宣言されている必要があるのでstd::functionを使う
	std::function<Ptr<Node>(void)> Expr, Equality, Relational, Add, Mul, Unary, Primary;

	Expr = [&]()->Ptr<Node> {
		return Equality();
	};

	Equality = [&]()->Ptr<Node> {

		Ptr<Node> node = Relational();

		for (;;) {
			if (Consume("=="))node = MakeNode(NODETYPE::EQUAL, node, Relational());
			else if (Consume("!="))node = MakeNode(NODETYPE::NOT_EQUAL, node, Relational());
			else return node;
		}

	};

	Relational = [&]()->Ptr<Node> {

		Ptr<Node> node = Add();

		for (;;) {
			if (Consume("<"))node = MakeNode(NODETYPE::LESS, node, Add());
			else if (Consume("<="))node = MakeNode(NODETYPE::LESS_OR_EQUAL, node, Add());
			else if (Consume(">"))node = MakeNode(NODETYPE::LESS, Add(), node);
			else if (Consume(">="))node = MakeNode(NODETYPE::LESS_OR_EQUAL, Add(), node);
			else return node;
		}

	};

	Add = [&]()->Ptr<Node> {

		Ptr<Node> node = Mul();

		for (;;) {
			if (Consume("+"))node = MakeNode(NODETYPE::ADD, node, Mul());
			else if (Consume("-"))node = MakeNode(NODETYPE::SUB, node, Mul());
			else return node;
		}

	};

	Mul = [&]()->Ptr<Node> {

		Ptr<Node> node = Primary();

		for (;;) {
			if (Consume("*"))node = MakeNode(NODETYPE::MUL, node, Unary());
			else if (Consume("/"))node = MakeNode(NODETYPE::DIV, node, Unary());
			else return node;
		}

	};

	Unary = [&]()->Ptr<Node> {

		if (Consume("+"))return Primary();
		if (Consume("-"))return MakeNode(NODETYPE::SUB, MakeNum(0), Primary());
		return Primary();

	};

	Primary = [&]()->Ptr<Node> {

		if (Consume("(")) {
			Ptr<Node> node = Expr();
			assert(Consume(")"));
			return node;
		}
		assert(tokens[pos].type == TOKENTYPE::NUMBER);
		return MakeNum(std::stoi(tokens[pos++].string)); //値を返したあとにposをインクリメント

	};

	root = Expr();

}

void ParseTest() {

	//assert(root->type == NODETYPE::NOT_EQUAL);
	Debug(root->lhs->num.value());

}

void GenerateAssembly() {

	std::string res = ".intel_syntax noprefix\n.global main\nmain:\n";

	std::function<void(Ptr<Node>)> recursive = [&](Ptr<Node> node)->void {

		if (node->type == NODETYPE::NUMBER) {
			std::string tmp = "\tpush " + std::to_string(node->num.value()) + "\n";
			res += tmp;
			return;
		}

		recursive(node->lhs);
		recursive(node->rhs);

		res += "\tpop rdi\n";
		res += "\tpop rax\n";

		switch (node->type) {
		case NODETYPE::ADD:
			res += "\tadd rax, rdi\n";
			break;

		case NODETYPE::SUB:
			res += "\tsub rax, rdi\n";
			break;

		case NODETYPE::MUL:
			res += "\tmul rax, rdi\n";
			break;

		case NODETYPE::DIV:
			res += "\tcqo\n";
			res += "\tidiv rdi\n";
			break;
		}

		res += "\tpush rax\n";

	};

	recursive(root);

	res += "\tpop rax\n";
	res += "\tret\n";

	assembly_code = res;

}

void CompileAssembly() {

	std::ofstream assembly_file{ "result.s" };
	assembly_file << assembly_code;
	assembly_file.close();

}

int main() {

	Tokenize();
	//TokenizeTest();
	Parse();
	//ParseTest();
	GenerateAssembly();
	CompileAssembly();

	//std::cout << assemblyCode;


}


/*

【備忘録】
expr       = equality
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | "(" expr ")"

*/
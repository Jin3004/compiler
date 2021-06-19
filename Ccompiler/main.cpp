#include <iostream>
#include <string>
#include <array>
#include <optional>
#include <vector>
#include <memory>
#include <functional>
#include <cassert>
#include <fstream>
#include <stdlib.h>
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
	ASSIGN,// =
	LOCAL_VAR,
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
	std::optional<int> num; //葉ノードが整数の場合には数字を持つ
	std::optional<int> offset; //葉ノードがローカル変数のときにはオフセットを持つ

	Node() {
		type = NODETYPE::NONE;
		rhs = nullptr;
		lhs = nullptr;
		num = std::nullopt;
		offset = std::nullopt;
	}

};


//グローバル変数
std::string src = "20 == 1 + 3 * 6 + 1";
std::vector<std::string> symbols = { "(", ")", "+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">=", "!", "=", ";" };
std::vector<std::string> keywords = { "int", "for" };
std::array<char, 3> white_space = { ' ', '\n', '\t' };
std::vector<Token> tokens{};
std::vector<Ptr<Node>> program;
std::string assembly_code = "";


namespace Utility {

	template<typename T = char, typename U = std::vector<char>>
	bool FullSearch(T obj, U& container) {
		size_t size = std::size(container);
		for (size_t i = 0; i < size; ++i) {
			if (obj == container[i])return true;
		}
		return false;
	}

	//完全二分木の描画 PrintBinaryTree(root, 0)で使う
	void PrintBinaryTree(Ptr<Node> node, int space) {

		constexpr int COUNT = 7;
		if (node == nullptr)return;

		space += COUNT;

		PrintBinaryTree(node->rhs, space);

		std::cout << "\n";
		for (int i = COUNT; i < space; ++i)std::cout << " ";

		if (node->type == NODETYPE::NUMBER)std::cout << node->num.value() << "\n";
		else std::cout << enum_name(node->type) << "\n";

		PrintBinaryTree(node->lhs, space);

	}

}

void Tokenize() {

	TOKENTYPE transition = TOKENTYPE::NONE;
	TOKENTYPE prev_transition = transition;
	size_t size = src.length();
	src += ' ';
	std::string tmp_string = "";

	auto MakeToken = [&]() {
		Token tmp;
		tmp.string = tmp_string;
		tmp.type = transition;
		tokens.push_back(tmp);
		tmp_string = "";
	};

	for (size_t pos = 0; pos < size; ++pos) {

		char cur = src[pos];
		char next = src[pos + 1];
		prev_transition = transition;

		if (transition == TOKENTYPE::NONE) {

			if (Utility::FullSearch(cur, white_space))continue;

			if ('0' <= cur && cur <= '9')transition = TOKENTYPE::NUMBER;
			else if (cur == '\'')transition = TOKENTYPE::CHARACTER;
			else if (cur == '\"')transition = TOKENTYPE::STRING;
			else if (('a' <= cur && cur <= 'z') || ('A' <= cur && cur <= 'Z') || (cur == '_'))transition = TOKENTYPE::IDENTIFIER;
			else  if (Utility::FullSearch(std::string{} +cur, symbols)) {
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
			if (!Utility::FullSearch(tmp_string + next, symbols))will_make_token = true;
		}

		if (will_make_token) {
			MakeToken();
			transition = TOKENTYPE::NONE;
			continue;
		}

	}

}

void TokenizeTest() {

	size_t token_size = tokens.size();

	for (size_t i = 0; i < token_size; ++i) {
		std::cout << enum_name(tokens[i].type) << ": " << tokens[i].string << "\n";
	}

}

void Parse() {

	size_t pos = 0; //今何番目のトークンを参照しているのか。
	auto Consume = [&](std::string str)->bool {
		if (pos >= tokens.size())return false;
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
	std::function<Ptr<Node>(void)> Statement, Expr, Assign, Equality, Relational, Add, Mul, Unary, Primary;

	Statement = [&]()->Ptr<Node> {
	
		Ptr<Node> node = Expr();
		assert(Consume(";"));

		return node;

	};

	Expr = [&]()->Ptr<Node> {
		return Equality();
	};

	Assign = [&]()->Ptr<Node> {

		Ptr<Node> node = Equality();

		for (;;) {
			if (Consume("="))node = MakeNode(NODETYPE::ASSIGN, node, Equality());
			else return node;
		}

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

	auto Program = [&]()->void {

		while (pos < tokens.size()) {
			Ptr<Node> tmp = Statement();
			program.push_back(tmp);
		}
	};

}

void ParseTest() {

	Utility::PrintBinaryTree(root, 0); //抽象構文木の出力

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
			res += "\tadd rax, rdi\n\n";
			break;

		case NODETYPE::SUB:
			res += "\tsub rax, rdi\n\n";
			break;

		case NODETYPE::MUL:
			res += "\timul rax, rdi\n\n";
			break;

		case NODETYPE::DIV:
			res += "\tcqo\n";
			res += "\tidiv rdi\n\n";
			break;

		case NODETYPE::EQUAL:
			res += "\tcmp rax, rdi\n";
			res += "\tsete al\n";
			res += "\tmovzb rax, al\n\n";
			break;

		case NODETYPE::NOT_EQUAL:
			res += "\tcmp rax, rdi\n";
			res += "\tsetne al\n";
			res += "\tmovzb rax, al\n\n";
			break;

		case NODETYPE::LESS:
			res += "\tcmp rax, rdi\n";
			res += "\tsetl al\n";
			res += "\tmovzb rax, al\n\n";
			break;

		case NODETYPE::LESS_OR_EQUAL:
			res += "\tcmp rax, rdi\n";
			res += "\tsetle al\n";
			res += "\tmovzb rax, al\n\n";
			break;

		}

		res += "\tpush rax\n\n";

	};

	recursive(root);

	res += "\tpop rax\n";
	res += "\tret\n";

	assembly_code = res;
}

void Assemble() {

	//出力されたアセンブリコードをresult.sに保存する
	std::ofstream assembly_file{ "result.s" };
	assembly_file << assembly_code;
	assembly_file.close();

	//result.sをバイナリファイルに変換する
	int res = system("wsl cc -o result result.s");
	if (res != 0)Debug("Assembling didn't go well.\n");

}

//生成された実行ファイルを実行し、終了コードを返す
int Run() {
	int res = system("wsl ./result");
	return res;
}

int main() {

	Tokenize();
	//TokenizeTest();
	Parse();
	//ParseTest();
	GenerateAssembly();

	//std::cout << assembly_code << "\n";

	Assemble();
	std::cout << Run();

}


/*

【備忘録】
program    = statement*
statement  = expr ";"
expr       = assign
assign     = equality ("=" equality)*
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary
primary    = num | "(" expr ")"

*/
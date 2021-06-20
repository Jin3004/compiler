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
#include <list>
#include <algorithm>
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

class LocalVar {
public:
	std::string name = ""; //変数名
	int offset = 0; //RBPからのオフセット
};


//グローバル変数
std::string src = "hoge = 12; fuga = 2 * 7 + 2;";
std::vector<std::string> symbols = { "(", ")", "+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">=", "!", "=", ";" };
std::vector<std::string> keywords = { "int", "for" };
std::array<char, 3> white_space = { ' ', '\n', '\t' };
std::vector<Token> tokens{};
std::vector<Ptr<Node>> statements;
std::string assembly_code = "";
std::list<LocalVar> local_vars;


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
	auto ConsumeByString = [&](std::string str)->bool {
		if (pos >= tokens.size())return false;
		if (tokens[pos].string == str) {
			++pos;
			return true;
		}
		return false;
	};

	auto ConsumeByType = [&](TOKENTYPE type)->bool {
		if (pos >= tokens.size())return false;
		if (tokens[pos].type == type) {
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

	auto MakeLocalVar = [&](std::string identifier)->Ptr<Node> {

		Ptr<Node> node = std::make_shared<Node>();

		//既にこの変数が登録されているかどうかを調べる
		auto find_res = std::find_if(local_vars.begin(), local_vars.end(), [&](LocalVar var) {return var.name == identifier; });

		if (find_res == local_vars.end()) {
			
			//新しくこの変数をlocal_varsに追加する
			LocalVar tmp;
			tmp.name = identifier;
			//最初に登録する変数のオフセットは8bytes
			if (local_vars.size() == 0) {
				tmp.offset = 8;
			}
			else {
				tmp.offset = local_vars.back().offset + 8;
			}
			node->offset = tmp.offset;
			local_vars.push_back(tmp);

		}
		else {
			node->offset = find_res->offset;
		}

		node->type = NODETYPE::LOCAL_VAR;
		return node;

	};


	//以下はそれぞれ前方宣言されている必要があるのでstd::functionを使う
	std::function<Ptr<Node>(void)> Statement, Expr, Assign, Equality, Relational, Add, Mul, Unary, Primary;

	Statement = [&]()->Ptr<Node> {

		Ptr<Node> node = Expr();
		assert(ConsumeByString(";"));

		return node;

	};

	Expr = [&]()->Ptr<Node> {
		return Assign();
	};

	Assign = [&]()->Ptr<Node> {

		Ptr<Node> node = Equality();

		for (;;) {
			if (ConsumeByString("="))node = MakeNode(NODETYPE::ASSIGN, node, Equality());
			else return node;
		}

	};

	Equality = [&]()->Ptr<Node> {

		Ptr<Node> node = Relational();

		for (;;) {
			if (ConsumeByString("=="))node = MakeNode(NODETYPE::EQUAL, node, Relational());
			else if (ConsumeByString("!="))node = MakeNode(NODETYPE::NOT_EQUAL, node, Relational());
			else return node;
		}

	};

	Relational = [&]()->Ptr<Node> {

		Ptr<Node> node = Add();

		for (;;) {
			if (ConsumeByString("<"))node = MakeNode(NODETYPE::LESS, node, Add());
			else if (ConsumeByString("<="))node = MakeNode(NODETYPE::LESS_OR_EQUAL, node, Add());
			else if (ConsumeByString(">"))node = MakeNode(NODETYPE::LESS, Add(), node);
			else if (ConsumeByString(">="))node = MakeNode(NODETYPE::LESS_OR_EQUAL, Add(), node);
			else return node;
		}

	};

	Add = [&]()->Ptr<Node> {

		Ptr<Node> node = Mul();

		for (;;) {
			if (ConsumeByString("+"))node = MakeNode(NODETYPE::ADD, node, Mul());
			else if (ConsumeByString("-"))node = MakeNode(NODETYPE::SUB, node, Mul());
			else return node;
		}

	};

	Mul = [&]()->Ptr<Node> {

		Ptr<Node> node = Primary();

		for (;;) {
			if (ConsumeByString("*"))node = MakeNode(NODETYPE::MUL, node, Unary());
			else if (ConsumeByString("/"))node = MakeNode(NODETYPE::DIV, node, Unary());
			else return node;
		}

	};

	Unary = [&]()->Ptr<Node> {

		if (ConsumeByString("+"))return Primary();
		if (ConsumeByString("-"))return MakeNode(NODETYPE::SUB, MakeNum(0), Primary());
		return Primary();

	};

	Primary = [&]()->Ptr<Node> {

		if (ConsumeByString("(")) {
			Ptr<Node> node = Expr();
			assert(ConsumeByString(")"));
			return node;
		}

		if (tokens[pos].type == TOKENTYPE::IDENTIFIER) {
			return MakeLocalVar(tokens[pos++].string);
		}

		assert(tokens[pos].type == TOKENTYPE::NUMBER);
		return MakeNum(std::stoi(tokens[pos++].string)); //値を返したあとにposをインクリメント

	};

	auto Program = [&]()->void {

		while (pos < tokens.size()) {
			Ptr<Node> tmp = Statement();
			statements.push_back(tmp);
		}
	};

	Program();

}

void ParseTest() {

	Utility::PrintBinaryTree(statements[1], 0); //抽象構文木の出力

}

void GenerateAssembly() {

	std::string res = ".intel_syntax noprefix\n.global main\nmain:\n";

	res += "#Prologue\n";
	res += "\tpush rbp\n\tmov rbp, rsp\n\tsub rsp, 208\n\n\n\n\n";


	//ノードが変数の場合、その変数のアドレスをプッシュする
	auto ReferToVar = [&](Ptr<Node> node)->void {

		assert(node->type == NODETYPE::LOCAL_VAR);

		res += "\tmov rax, rbp\n";
		res += "\tsub rax, ";
		res += std::to_string(node->offset.value());
		res += "\n";
		res += "\tpush rax\n\n";

	};



	std::function<void(Ptr<Node>)> Recursive = [&](Ptr<Node> node)->void {

		switch (node->type) {
		case NODETYPE::NUMBER:
			res += "\tpush ";
			res += std::to_string(node->num.value());
			res += "\n";
			return;

		case NODETYPE::LOCAL_VAR:
			ReferToVar(node);
			//右辺値として変数が用いられる場合には、値をロードする
			res += "\tpop rax\n";
			res += "\tmov rax, [rax]\n";
			res += "\tpush rax\n\n";
			return;

		case NODETYPE::ASSIGN:
			ReferToVar(node->lhs);
			Recursive(node->rhs);
			//上の段階で変数のメモリアドレスと右辺値がプッシュされている
			res += "\tpop rdi\n";
			res += "\tpop rax\n";
			res += "\tmov [rax], rdi\n";
			res += "\tpush rdi\n\n";
			return;
		}

		Recursive(node->lhs);
		Recursive(node->rhs);

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

	size_t statement_num = statements.size();
	for (size_t i = 0; i < statement_num; ++i) {
		Recursive(statements[i]);
		res += "\tpop rax\n\n";
	}

	res += "\tmov rsp, rbp\n";
	res += "\tpop rbp\n";
	res += "\tret\n";

	assembly_code = res;
}

void GenerateAssemblyTest() {
	Debug(assembly_code);
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
	//GenerateAssemblyTest();
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
primary    = num | identifier | "(" expr ")"

*/
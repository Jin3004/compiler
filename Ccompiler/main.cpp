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
#include <variant>
#include "magic_enum/include/magic_enum.hpp"

#define Debug(var) std::cout << var << "\n"

//クラスとかの宣言
template<class T>
using Ptr = std::shared_ptr<T>;
using namespace magic_enum;

/*前方宣言(必要なら)*/


/*----------------*/


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
	RETURN,// return
	LOCAL_VAR,
	NUMBER,
	BLOCK,// {}
	IF,
	WHILE,
	FOR,
	FUNCTION_CALL,// 関数呼び出し
	FUNCTION_DEFINITION,//関数定義
	ADDRESS,//&
	DEREFRENCE,//*
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

class LocalVarData {
public:
	std::string name = ""; //変数名
	int offset = 0; //RBPからのオフセット
};


class Node {
public:
	NODETYPE type;
	std::vector<Ptr<Node>> data;//子ノードの格納する配列

	std::string function_name;
	int num;
	size_t offset, arg_num, allocated_size;

	Node() {
		num = 0;
		offset = 0;
		arg_num = 0;
		allocated_size = 0;
		type = NODETYPE::NONE;
	}
};

//グローバル変数
std::string src = "";
std::vector<std::string> symbols = { "(", ")", "+", "-", "*", "/", "==", "!=", "<", "<=", ">", ">=", "!", "=", ";", "{", "}", ",", "&" };
std::vector<std::string> keywords = { "int", "for", "return" };
std::array<char, 3> white_space = { ' ', '\n', '\t' };
std::vector<Token> tokens{};
std::vector<Ptr<Node>> program;
std::string assembly_code = "";
std::list<LocalVarData> local_var_table;


namespace Utility {

	template<typename T = char, typename U = std::vector<char>>
	bool FullSearch(T obj, U& container) {
		size_t size = std::size(container);
		for (size_t i = 0; i < size; ++i) {
			if (obj == container[i])return true;
		}
		return false;
	}

	/*

	//完全二分木の描画 PrintBinaryTree(root, 0)で使う
	void PrintBinaryTree(Ptr<Node> node, int space) {

		constexpr int count = 7;
		if (node == nullptr)return;

		space += count;

		PrintBinaryTree(node->, space);

		std::cout << "\n";
		for (int i = count; i < space; ++i)std::cout << " ";

		if (node->type == NODETYPE::NUMBER)std::cout << node->data.num << "\n";
		else std::cout << enum_name(node->type) << "\n";

		PrintBinaryTree(node->lhs, space);

	}
	*/

}

void LoadSourceFromFile(std::string file_name) {
	std::ifstream ifs{ file_name };

	std::string tmp = "";
	while (std::getline(ifs, tmp)) {
		src += tmp;
		src += "\n";
	}

	ifs.close();
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

	size_t token_pos = 0; //今何番目のトークンを参照しているのか。
	auto ConsumeByString = [&](std::string str)->bool {
		if (token_pos >= tokens.size())return false;
		if (tokens[token_pos].string == str) {
			++token_pos;
			return true;
		}
		return false;
	};

	auto ConsumeByType = [&](TOKENTYPE type)->bool {
		if (token_pos >= tokens.size())return false;
		if (tokens[token_pos].type == type) {
			++token_pos;
			return true;
		}
		return false;
	};

	//演算子の場合子供が必ず２つなのでそれ用の関数オブジェクトを作っておく[0] -> 左辺, [1] -> 右辺
	auto MakeBinaryNode = [](NODETYPE type, Ptr<Node> lhs, Ptr<Node> rhs)->Ptr<Node> {
		Ptr<Node> node = std::make_shared<Node>();
		node->type = type;
		node->data.push_back(lhs);
		node->data.push_back(rhs);
		return node;
	};

	auto MakeNum = [](int num)->Ptr<Node> {
		Ptr<Node> node = std::make_shared<Node>();
		node->num = num;
		node->type = NODETYPE::NUMBER;
		return node;
	};

	auto MakeFunctionCallNode = [&](std::string identifier)->Ptr<Node> {
		Ptr<Node> node = std::make_shared<Node>();
		node->function_name = identifier;
		node->type = NODETYPE::FUNCTION_CALL;
		return node;
	};


	//以下はそれぞれ前方宣言されている必要があるのでstd::functionを使う
	std::function<Ptr<Node>(void)> FunctionDefinition, Statement, Expr, Assign, Equality, Relational, Add, Mul, Unary, Primary, Lvalue;

	FunctionDefinition = [&]()->Ptr<Node> {

		Ptr<Node> node = std::make_shared<Node>();
		node->type = NODETYPE::FUNCTION_DEFINITION;
		assert(tokens[token_pos].type == TOKENTYPE::IDENTIFIER);
		node->function_name = tokens[token_pos++].string;//関数名の処理	

		//引数の読み込み
		assert(ConsumeByString("("));
		if (!ConsumeByString(")")) {
			while (true) {
				node->data.push_back(Lvalue());
				++node->arg_num;
				if (ConsumeByString(")"))break;
				else assert(ConsumeByString(","));
			}
		}

		//ブロックの読み込み
		assert(ConsumeByString("{"));
		while (!ConsumeByString("}")) {
			node->data.push_back(Statement());
		}

		return node;

	};

	Statement = [&]()->Ptr<Node> {

		//[0] -> 条件式, [1] -> 文, [2] -> elseだったらその文
		if (ConsumeByString("if")) {

			//if文の実装

			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::IF;

			assert(ConsumeByString("("));
			node->data.push_back(Expr());
			assert(ConsumeByString(")"));

			node->data.push_back(Statement());

			//else文の実装
			if (ConsumeByString("else")) {
				node->data.push_back(Statement());
			}

			return node;

		}

		//[0] -> 条件式, [1] -> 文
		if (ConsumeByString("while")) {

			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::WHILE;

			assert(ConsumeByString("("));
			node->data.push_back(Expr());
			assert(ConsumeByString(")"));

			node->data.push_back(Statement());
			return node;

		}

		if (ConsumeByString("for")) {

			Ptr<Node> node = std::make_shared<Node>();
			node->data.resize(4, nullptr);
			node->type = NODETYPE::FOR;

			assert(ConsumeByString("("));

			if (!ConsumeByString(";")) {
				node->data[0] = Expr();
				ConsumeByString(";");
			}

			if (!ConsumeByString(";")) {
				node->data[1] = Expr();
				ConsumeByString(";");
			}

			if (!ConsumeByString(")")) {
				node->data[2] = Expr();
				ConsumeByString(")");
			}

			node->data[3] = Statement();

			return node;

		}

		//ブロックの実装
		if (ConsumeByString("{")) {
			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::BLOCK;

			while (!ConsumeByString("}")) {
				node->data.push_back(Statement());
			}
			return node;
		}

		//return文の実装 [0] -> returnの値
		if (ConsumeByString("return")) {
			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::RETURN;
			node->data.push_back(Expr());
			assert(ConsumeByString(";"));
			return node;
		}

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
			if (ConsumeByString("="))node = MakeBinaryNode(NODETYPE::ASSIGN, node, Equality());
			else return node;
		}

	};

	Equality = [&]()->Ptr<Node> {

		Ptr<Node> node = Relational();

		for (;;) {
			if (ConsumeByString("=="))node = MakeBinaryNode(NODETYPE::EQUAL, node, Relational());
			else if (ConsumeByString("!="))node = MakeBinaryNode(NODETYPE::NOT_EQUAL, node, Relational());
			else return node;
		}

	};

	Relational = [&]()->Ptr<Node> {

		Ptr<Node> node = Add();

		for (;;) {
			if (ConsumeByString("<"))node = MakeBinaryNode(NODETYPE::LESS, node, Add());
			else if (ConsumeByString("<="))node = MakeBinaryNode(NODETYPE::LESS_OR_EQUAL, node, Add());
			else if (ConsumeByString(">"))node = MakeBinaryNode(NODETYPE::LESS, Add(), node);
			else if (ConsumeByString(">="))node = MakeBinaryNode(NODETYPE::LESS_OR_EQUAL, Add(), node);
			else return node;
		}

	};

	Add = [&]()->Ptr<Node> {

		Ptr<Node> node = Mul();

		for (;;) {
			if (ConsumeByString("+"))node = MakeBinaryNode(NODETYPE::ADD, node, Mul());
			else if (ConsumeByString("-"))node = MakeBinaryNode(NODETYPE::SUB, node, Mul());
			else return node;
		}

	};

	Mul = [&]()->Ptr<Node> {

		Ptr<Node> node = Unary();

		for (;;) {
			if (ConsumeByString("*"))node = MakeBinaryNode(NODETYPE::MUL, node, Unary());
			else if (ConsumeByString("/"))node = MakeBinaryNode(NODETYPE::DIV, node, Unary());
			else return node;
		}

	};

	Unary = [&]()->Ptr<Node> {

		if (ConsumeByString("&")) {
			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::ADDRESS;
			node->data.push_back(Lvalue());
			return node;
		}
		if (ConsumeByString("*")) {
			Ptr<Node> node = std::make_shared<Node>();
			node->type = NODETYPE::DEREFRENCE;
			node->data.push_back(Unary());
			return node;
		}
		
		if (ConsumeByString("+"))return Primary();
		if (ConsumeByString("-"))return MakeBinaryNode(NODETYPE::SUB, MakeNum(0), Primary());
		return Primary();

	};

	Primary = [&]()->Ptr<Node> {

		if (ConsumeByString("(")) {
			Ptr<Node> node = Expr();
			assert(ConsumeByString(")"));
			return node;
		}

		if (tokens[token_pos].type == TOKENTYPE::IDENTIFIER) {

			Ptr<Node> node = nullptr;

			//もし識別子の次に(があったらそれは関数呼び出し
			if (tokens[token_pos + 1].string == "(") {

				node = MakeFunctionCallNode(tokens[token_pos++].string);
				assert(ConsumeByString("("));
				//もし引数があったら引数の読み込み
				if (!ConsumeByString(")")) {
					while (true) {
						node->data.push_back(Expr());
						if (ConsumeByString(")"))break;
						else assert(ConsumeByString(","));
					}
				}

			}//でなければ変数
			else {
				node = Lvalue();
			}

			return node;

		}

		assert(tokens[token_pos].type == TOKENTYPE::NUMBER);
		return MakeNum(std::stoi(tokens[token_pos++].string)); //値を返したあとにposをインクリメント

	};

	//変数
	Lvalue = [&]()->Ptr<Node> {

		Ptr<Node> node = std::make_shared<Node>();
		node->type = NODETYPE::LOCAL_VAR;
		assert(tokens[token_pos].type == TOKENTYPE::IDENTIFIER);
		std::string var_name = tokens[token_pos++].string;

		auto find_res = std::find_if(local_var_table.begin(), local_var_table.end(), [&](LocalVarData var) {return var.name == var_name; });
		if (find_res == local_var_table.end()) {
			
			LocalVarData tmp;
			tmp.name = var_name;
			if (local_var_table.size() == 0)tmp.offset = 8;
			else tmp.offset = local_var_table.back().offset + 8;

			node->offset = tmp.offset;
			local_var_table.push_back(tmp);
		}
		else {
			node->offset = find_res->offset;
		}

		return node;

	};

	while (token_pos < tokens.size()) {
		program.push_back(FunctionDefinition());
		program.back()->allocated_size = local_var_table.size() * 8;
		local_var_table = {};//関数が変わるごとに変数テーブルをクリアする	
	}

}

void ParseTest() {

	//Utility::PrintBinaryTree(program->data.statements[0], 0); //抽象構文木の出力

}

void GenerateAssembly() {

	static int label_count = 0;
	static std::array<std::string, 6> arg_registers = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };//引数用のレジスターの名前を保持
	std::string res = ".intel_syntax noprefix\n.global main\n\n\n";

	std::function<void(Ptr<Node>)> Recursive;

	auto MakePrologue = [](Ptr<Node> node)->std::string {
		assert(node->type == NODETYPE::FUNCTION_DEFINITION);
		std::string prologue;
		prologue += "\n#Prologue\n";
		prologue += "\tpush rbp\n\tmov rbp, rsp\n";
		for (size_t i = 0; i < node->arg_num; ++i) {
			prologue += ("\tpush " + arg_registers[i] + "\n");
		}
		prologue += "\tsub rsp, ";
		prologue += std::to_string(node->allocated_size - node->arg_num * 8);
		prologue += "\n#End of Prologue\n\n";
		return prologue;
	};

	auto MakeLabel = [](int count)->std::string {
		std::string str = ".L" + std::to_string(count);
		return str;
	};

	//ノードが変数の場合、その変数のアドレスをプッシュする
	auto ReferToVar = [&](Ptr<Node> node)->void {

		assert(node->type == NODETYPE::LOCAL_VAR);

		res += "\tmov rax, rbp\n";
		res += "\tsub rax, ";
		res += std::to_string(node->offset);
		res += "\n";
		res += "\tpush rax\n\n";

	};

	//制御構文の実装

	auto IfImplement = [&](Ptr<Node> node)->void {

		assert(node->type == NODETYPE::IF);
		Recursive(node->data[0]);
		//この段階でスタックトップに条件式の結果が入っている
		res += "\tpop rax\n";
		res += "\tcmp rax, 0\n";
		res += "\tje ";

		++label_count;
		std::string label1 = MakeLabel(label_count);

		res += (label1 + "\n");
		Recursive(node->data[1]);

		//elseじゃなかったら
		if (node->data.size() != 3) {
			res += (label1 + ":\n");
		}
		else {
			++label_count;
			std::string label2 = MakeLabel(label_count);
			res += "\tjmp ";
			res += (label2 + "\n");
			res += (label1 + ":\n");
			Recursive(node->data[2]);
			res += (label2 + ":\n");
		}

	};

	auto WhileImplement = [&](Ptr<Node> node) {

		++label_count;
		std::string label1 = MakeLabel(label_count);
		++label_count;
		std::string label2 = MakeLabel(label_count);

		res += (label1 + ":\n");

		Recursive(node->data[0]);

		res += "\tpop rax\n";
		res += "\tcmp rax, 0\n";
		res += ("\tje " + label2 + "\n");

		Recursive(node->data[1]);

		res += ("\tjmp " + label1 + "\n");
		res += (label2 + ":\n");

	};

	auto ForImplement = [&](Ptr<Node> node) {

		if (node->data[0] != nullptr)Recursive(node->data[0]);
		++label_count;
		std::string label1 = MakeLabel(label_count);
		++label_count;
		std::string label2 = MakeLabel(label_count);

		res += (label1 + ":\n");

		if (node->data[1] != nullptr)Recursive(node->data[1]);

		res += "\tpop rax\n";
		res += "\tcmp rax, 0\n";
		res += ("\tje " + label2 + "\n");

		Recursive(node->data[3]);
		if (node->data[2] != nullptr)Recursive(node->data[2]);

		res += ("\tjmp " + label1 + "\n");
		res += (label2 + ":\n");

	};

	//

	//関数呼び出しの実装

	auto FunctionCallImplement = [&](Ptr<Node> node) {

		auto arg_size = node->data.size();
		assert(arg_size <= 6);//引数は6つまで受け付ける
		for (size_t i = 0; i < arg_size; ++i) {
			Recursive(node->data[i]);//引数の値を評価
			res += "\tpop rax\n";
			res += ("\tmov " + arg_registers[i] + ", rax\n");
		}
		res += ("\tcall " + node->function_name + "\n");

	};

	//

	//関数定義の実装

	auto FunctionDefinitionImplement = [&](Ptr<Node> node) {

		res += (node->function_name + ":\n");

		res += MakePrologue(node);
		for (size_t i = 0; i < node->arg_num; ++i)res += ("push " + arg_registers[i] + "\n");
		for (size_t i = node->arg_num; i < node->data.size(); ++i)Recursive(node->data[i]);

	};
	
	//

	Recursive = [&](Ptr<Node> node)->void {

		switch (node->type) {
		case NODETYPE::ADDRESS:
			ReferToVar(node->data[0]);
			return;

		case NODETYPE::DEREFRENCE:
			Recursive(node->data[0]);
			res += "\tpop rax\n";
			res += "\tmov rax, [rax]\n";
			res += "\tpush rax\n";
			return;

		case NODETYPE::FUNCTION_DEFINITION:
			FunctionDefinitionImplement(node);
			return;

		case NODETYPE::BLOCK:
			for (auto i = 0; i < node->data.size(); ++i) {
				Recursive(node->data[i]);
			}
			return;

		case NODETYPE::NUMBER:
			res += "\tpush ";
			res += std::to_string(node->num);
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
			ReferToVar(node->data[0]);
			Recursive(node->data[1]);
			//上の段階で変数のメモリアドレスと右辺値がプッシュされている
			res += "\tpop rdi\n";
			res += "\tpop rax\n";
			res += "\tmov [rax], rdi\n";
			res += "\tpush rdi\n\n";
			return;

		case NODETYPE::RETURN:
			res += "\n#Epilogue\n";
			Recursive(node->data[0]);
			if(node->data[0]->type != NODETYPE::FUNCTION_CALL)res += "\tpop rax\n";
			res += "\tmov rsp, rbp\n";
			res += "\tpop rbp\n";
			res += "\tret\n";
			res += "#End of Epilogue\n\n";
			return;

		case NODETYPE::IF:
			IfImplement(node);
			return;

		case NODETYPE::WHILE:
			WhileImplement(node);
			return;

		case NODETYPE::FOR:
			ForImplement(node);
			return;

		case NODETYPE::FUNCTION_CALL:
			FunctionCallImplement(node);
			return;

		}

		//これより下は数値計算

		Recursive(node->data[0]);
		Recursive(node->data[1]);

		res += "\tpop rdi\n";
		res += "\tpop rax\n";

		switch (node->type) {
		case NODETYPE::ADD:
			res += "\tadd rax, rdi\n";
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
			res += "\tmovzb rax, al\n";
			break;

		case NODETYPE::NOT_EQUAL:
			res += "\tcmp rax, rdi\n";
			res += "\tsetne al\n";
			res += "\tmovzb rax, al\n";
			break;

		case NODETYPE::LESS:
			res += "\tcmp rax, rdi\n";
			res += "\tsetl al\n";
			res += "\tmovzb rax, al\n";
			break;

		case NODETYPE::LESS_OR_EQUAL:
			res += "\tcmp rax, rdi\n";
			res += "\tsetle al\n";
			res += "\tmovzb rax, al\n";
			break;

		}

		res += "\tpush rax\n";

	};

	for (auto itr = program.begin(); itr != program.end(); ++itr) {
		Recursive(*itr);
	}

	assembly_code = res;
}

void GenerateAssemblyTest() {
	Debug(assembly_code);
}

void RunAssemblyForTest() {

	{

		int res = system("wsl cc -o test test.s");
		if (res != 0)Debug("Assembling didn't go well.");

	}

	{

		int res = system("wsl ./test");
		Debug(res);
	}

}

void Assemble() {

	//出力されたアセンブリコードをresult.sに保存する
	std::ofstream assembly_file{ "result.s" };
	assembly_file << assembly_code;
	assembly_file.close();

	//result.sをバイナリファイルに変換する
	int res = system("wsl cc -o result result.s");
	if (res != 0)Debug("Assembling didn't go well.");

}

//生成された実行ファイルを実行し、終了コードを返す
void Run() {
	int res = system("wsl ./result");
	Debug(res);
}

int main() {

	LoadSourceFromFile("input.c");
	Tokenize();
	TokenizeTest();
	Parse();
	//ParseTest();
	GenerateAssembly();
	//GenerateAssemblyTest();
	Assemble();
	Run();
	//RunAssemblyForTest();

}


/*

【備忘録】
program    = function_definition*
function_definition = ident "(" 引数 ")" "{" statement* "}"
statement    = expr ";" | "return" expr ";" | "if" "(" expr ")" statement ("else" statement)* | "while" "(" expr ")" statement | "for" "(" expr? ";" expr? ";" ")" statement | "{" statement* "}"
expr       = assign
assign     = equality ("=" equality)*
equality   = relational ("==" relational | "!=" relational)*
relational = add ("<" add | "<=" add | ">" add | ">=" add)*
add        = mul ("+" mul | "-" mul)*
mul        = unary ("*" unary | "/" unary)*
unary      = ("+" | "-")? primary | "*" lvalue | "&" lvalue
primary    = num | "(" expr ")" | identifier("(" ")")? | lvalue
lvalue = identifier

*/
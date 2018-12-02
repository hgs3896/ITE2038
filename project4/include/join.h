#ifndef __JOIN_H__
#define __JOIN_H__

#include "types.h"
#include <array>
#include <string>
#include <forward_list>
#include <set>
#include <queue>
#include <map>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <initializer_list>
#include <vector>
#include <algorithm>
#include <thread>

class Operator;
class SumOperator;
class LoadOperator;
class JoinOperator;
class HashJoinOperator;
class SortMergeJoinOperator;
class ParseTree;
class TableStat;

struct Table
{
	using ID = int;
	using Col = int;
	using Pos = int;
	
	using ColumnInfo = std::unordered_map<Col, Pos>;
	using Info = std::unordered_map<ID, ColumnInfo>;
	using Record = std::vector<record_val_t>;
	using Records = std::vector<Record>;
	
	// 전체 컬럼 개수
	int totalCol;
	// info[tid][col] = tid의 col의 physical index
	Info info;
	// records[info[tid][col]] = tid의 col의 레코드
	Records records;

	Table();
	Table(const Table& table);
	Table(Table&& table);
	Table& operator=(const Table& table);
	Table& operator=(Table&& table);
};

class Operator
{
protected:
	Table table;
protected:
	Operator(const Operator& copy) = delete;
	Operator& operator=(const Operator& copy) = delete;
public:
	Operator();
	Operator(Operator&& copy);
	Operator& operator=(Operator&& copy);
	virtual void execute() = 0;
	Table&& getTable();
	virtual ~Operator();
};

class LoadOperator : public Operator
{
private:
	int tableID;
public:
	LoadOperator(int tableID);
	LoadOperator(LoadOperator&& copy);
	LoadOperator& operator=(LoadOperator&& copy);
	int getTableID() const;
	void execute();	
	virtual ~LoadOperator();
};

class SumOperator : public Operator
{
private:
	record_val_t sum;
	JoinOperator* op;	
public:
	SumOperator(JoinOperator* op);
	SumOperator(SumOperator&& copy);
	SumOperator& operator=(SumOperator&& copy);

	record_val_t getSum() const;

	void execute();
	
	virtual ~SumOperator();
};

// *
class JoinOperator : public Operator
{
public:
	struct JoinInfo
	{
		/* Join Information
		 * {
		 *   { t1의 table id, t1 join key의 index }
		 *   { t2의 table id, t2 join key의 index }
		 * }
		 */
		int tid1, tid2, key1, key2, rrf;
		constexpr bool operator<(const JoinInfo& rhs) const
		{
			return this->rrf > rhs.rrf;
		}
	};
	using JoinList = std::forward_list<JoinInfo>;
	
protected:
	Operator *left, *right;
	JoinList joinList;
	
public:
	JoinOperator();
	JoinOperator(JoinInfo&& joinInfo);
	JoinOperator(JoinOperator&& copy);
	JoinOperator& operator=(JoinOperator&& copy);
	void setLeft(Operator* operation);
	void setRight(Operator* operation);
	void addJoinInfo(JoinInfo joinInfo);
	JoinList&& getJoinList();
	void execute();
	virtual ~JoinOperator();
};

class HashJoinOperator : public JoinOperator
{
public:
	using HashTable = std::unordered_map<record_val_t, Table::Records>;
	HashJoinOperator();
	HashJoinOperator(JoinInfo&& joinInfo);
	HashJoinOperator(HashJoinOperator&& copy);
	HashJoinOperator& operator=(HashJoinOperator&& copy);

	void execute();
	virtual ~HashJoinOperator();
};

class ParseTree
{
private:
	SumOperator *op;
public:
	ParseTree(const std::string& command);
	record_val_t execute();
	~ParseTree();
};

class TableStat {
public:
	// dist[tableID][colNum][key] == 해당 key의 개수
	using ColumnDist = std::map<record_val_t, size_t>;
	using TableDist = std::unordered_map<Table::Col, ColumnDist>;
	using Dist = std::unordered_map<Table::ID, TableDist>;
	/* 
	 * { 
	 *	 table1 ID, table1's column number, table2 ID, table2's column number,
	 *   table1 min key, table1 max key, table2 min key, table2 max key
	 * }
	 */
	using JoinInfo = std::initializer_list<record_val_t>;
private:	
	static Dist dist;
public:
	static void analyze(Table::ID tableID);
	static std::size_t getColumnCount(Table::ID tableID, Table::Col colNum);
	static std::size_t getInverseOfReductionFactor(JoinInfo joinInfo);
};


#endif
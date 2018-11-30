#include "join.h"
#include "wrapper_funcs.h"
#include "buffer_manager.h"

// Table

Table::Table()
	:totalCol(0), records(), info()
{
}

Table::Table(const Table& table)
	: totalCol(table.totalCol), records(table.records), info(table.info)
{	
}

Table::Table(Table&& table)
	: totalCol(table.totalCol), records(std::move(table.records)), info(std::move(table.info))
{
}

Table& Table::operator=(const Table& table)
{
	if ( &table != this )
	{
		totalCol = table.totalCol;
		records = table.records;
		info = table.info;
	}
	return *this;
}

Table& Table::operator=(Table&& table)
{
	if ( &table != this )
	{
		totalCol = table.totalCol;
		records = std::move(table.records);
		info = std::move(table.info);
	}
	return *this;
}

// Operator

Operator::Operator()
	:table()
{

}

Operator::Operator(Operator&& copy)
	:table(std::move(copy.table))
{
	
}

Operator& Operator::operator=(Operator&& copy)
{
	table = std::move(copy.table);
	return *this;
}

Table&& Operator::getTable()
{
	return std::move(table);
}

Operator::~Operator()
{

}

// LoadOperator

LoadOperator::LoadOperator(int tableID)
	:tableID(tableID), Operator()
{
	auto num_col = getNumOfCols(tableID);
	table.totalCol = num_col;
	for ( auto i = 1; i <= num_col; ++i )
		table.info[tableID][i] = i - 1;
}

LoadOperator::LoadOperator(LoadOperator&& copy)
	:tableID(copy.tableID), Operator(std::move(copy))
{

}

LoadOperator& LoadOperator::operator=(LoadOperator&& copy)
{
	tableID = copy.tableID;
	table = std::move(copy.table);
	return *this;
}

void LoadOperator::execute()
{
	auto next = HEADER_PAGE_OFFSET;

	auto buf = BufferManager::get_frame(tableID, PGNUM(next));
	auto page = BufferManager::get_page(buf, false);
	next = page->getRootPageOffset();
	BufferManager::put_frame(buf);
	
	if ( next == HEADER_PAGE_OFFSET )
		return;

	buf = BufferManager::get_frame(tableID, PGNUM(next));
	page = BufferManager::get_page(buf, false);

	while ( !page->isLeaf() )
	{
		next = page->getOffset(0);

		BufferManager::put_frame(buf);
		buf = BufferManager::get_frame(tableID, PGNUM(next));
		page = BufferManager::get_page(buf, false);
	}

	while ( true )
	{
		for ( auto idx = 0; idx < page->getNumOfKeys(); ++idx )
			table.records.emplace_back(std::move(page->getRecord(idx, getNumOfCols(tableID))));

		next = page->getOffset(DEFAULT_LEAF_ORDER - 1);
		if ( next == HEADER_PAGE_OFFSET )
			break;

		BufferManager::put_frame(buf);
		buf = BufferManager::get_frame(tableID, PGNUM(next));
		page = BufferManager::get_page(buf, false);
	}
	BufferManager::put_frame(buf);
}

int LoadOperator::getTableID() const
{
	return tableID;
}

LoadOperator::~LoadOperator()
{
}

// SumOperator

SumOperator::SumOperator(JoinOperator* op)
	:Operator(), op(op), sum(0)
{
	
}

SumOperator::SumOperator(SumOperator&& copy)
	:Operator(std::move(copy)), op(copy.op), sum(0)
{
	copy.op = nullptr;
}

SumOperator& SumOperator::operator=(SumOperator&& copy)
{
	op = copy.op;
	table = std::move(copy.table);
	sum = copy.sum;

	copy.op = nullptr;
	copy.sum = 0;
	return *this;
}

void SumOperator::execute()
{
	if ( auto join = dynamic_cast<JoinOperator*>(op) )
	{
		// 조인 키의 합을 계산한다.
		join->execute();
		table = std::move(join->getTable());
		auto joinList = std::move(join->getJoinList());
		std::vector<int> keySet;
		for ( const auto& tb : table.info )
			keySet.emplace_back(table.info[tb.first][1]);

		for ( const auto& record : table.records )
		{
			for ( const auto idx : keySet )
			{
				sum += record[idx];
			}				
		}
	}
}

record_val_t SumOperator::getSum() const
{
	return sum;
}

SumOperator::~SumOperator()
{
	delete op;
}

// JoinOperator

JoinOperator::JoinOperator()
	:Operator(), left(nullptr), right(nullptr), joinList()
{

}

JoinOperator::JoinOperator(JoinInfo&& joinInfo)
	:Operator(), left(nullptr), right(nullptr), joinList()
{
	joinList.emplace_front(std::move(joinInfo));
}

JoinOperator::JoinOperator(JoinOperator&& copy)
	: Operator(std::move(copy)), left(copy.left), right(copy.right)
	, joinList(std::move(copy.joinList))
{

}

JoinOperator::JoinList&& JoinOperator::getJoinList()
{
	return std::move(joinList);
}

JoinOperator& JoinOperator::operator=(JoinOperator&& copy)
{
	(Operator&)(*this) = std::move(copy);
	left = copy.left;
	right = copy.right;
	joinList = std::move(copy.joinList);	

	copy.left = nullptr;
	copy.right = nullptr;
	return *this;
}

void JoinOperator::setLeft(Operator* operation)
{
	left = operation;
}

void JoinOperator::setRight(Operator* operation)
{
	right = operation;
}

void JoinOperator::addJoinInfo(JoinInfo joinInfo)
{
	joinList.emplace_front(std::move(joinInfo));
}

void JoinOperator::execute()
{
	// Hash Join으로 실행
}

JoinOperator::~JoinOperator()
{
	delete left;
	delete right;
}

// HashJoinOperator

HashJoinOperator::HashJoinOperator()
	: JoinOperator()
{
}

HashJoinOperator::HashJoinOperator(JoinInfo&& joinInfo)
	: JoinOperator(std::move(joinInfo))
{

}

HashJoinOperator::HashJoinOperator(HashJoinOperator&& copy)
	:JoinOperator(std::move(copy))
{

}

HashJoinOperator& HashJoinOperator::operator=(HashJoinOperator&& copy)
{
	(JoinOperator&)(*this) = std::move(copy);
	return *this;
}

void HashJoinOperator::execute()
{
	left->execute();
	right->execute();

	HashJoinOperator::HashTable hashTable1, hashTable2;

	auto t1 = left->getTable();
	auto t2 = right->getTable();

	if ( auto join1 = dynamic_cast<JoinOperator*>(left) )
	{
		auto list = join1->getJoinList();
		joinList.insert_after(joinList.begin(), list.begin(), list.end());
	}

	if ( auto join2 = dynamic_cast<JoinOperator*>(right) )
	{
		auto list = join2->getJoinList();
		joinList.insert_after(joinList.begin(), list.begin(), list.end());
	}

	auto joinInfo = *joinList.cbegin();

	const auto idx_1 = t1.info[joinInfo.tid1][joinInfo.key1];
	const auto idx_2 = t2.info[joinInfo.tid2][joinInfo.key2];

	auto hashing = [](const int idx, const Table::Records& records, HashJoinOperator::HashTable& hashTable) {
		for ( const auto& record : records )
		{
			hashTable[record[idx]].push_back(record);
		}
	};

	auto matching = [this, &t1 = t1, &t2 = t2, &ht1 = hashTable1, &ht2 = hashTable2, idx_1, idx_2]() {
		table.totalCol = t1.totalCol + t2.totalCol;
		table.info.insert(t1.info.cbegin(), t1.info.cend());
		table.info.insert(t2.info.cbegin(), t2.info.cend());
		for ( auto tit = t2.info.cbegin(); tit != t2.info.cend(); ++tit )
		{
			for ( auto cit = tit->second.cbegin(); cit != tit->second.cend(); ++cit )
			{
				table.info[tit->first][cit->first] += t1.totalCol;
			}
		}

		for ( const auto& left : ht1)
		{
			if ( ht2.count(left.first) )
			{
				for ( const auto& left_record : left.second )
				{
					for ( const auto& right_record : ht2[left.first] )
					{
						Table::Record new_record;
						new_record.insert(new_record.end(), left_record.begin(), left_record.end());
						new_record.insert(new_record.end(), right_record.begin(), right_record.end());
						table.records.push_back(std::move(new_record));
					}
				}
			}
		}
	};
	
	std::thread th1 = std::thread(hashing, idx_1, std::ref(t1.records), std::ref(hashTable1));
	std::thread th2 = std::thread(hashing, idx_2, std::ref(t2.records), std::ref(hashTable2));

	th1.join();
	th2.join();

	std::thread th3 = std::thread(matching);
	th3.join();
}

HashJoinOperator::~HashJoinOperator()
{
}

ParseTree::ParseTree(const std::string& command)
	: op(nullptr)
{
	// Query Parsing

	auto fetchJoinInfo = [](const std::string joinQuery) -> JoinOperator::JoinInfo {
		auto pos_eq = joinQuery.find("=");

		auto pos_dot = joinQuery.find(".");
		auto tableID = joinQuery.substr(0, pos_dot);
		auto joinKey = joinQuery.substr(pos_dot + 1, pos_eq - (pos_dot + 1));
		auto tid1 = std::stoi(tableID);
		auto key1 = std::stoi(joinKey);

		pos_dot = joinQuery.find(".", pos_eq);
		tableID = joinQuery.substr(pos_eq + 1, pos_dot - (pos_eq + 1));
		joinKey = joinQuery.substr(pos_dot + 1);
		auto tid2 = std::stoi(tableID);
		auto key2 = std::stoi(joinKey);

		return {
			tid1, tid2,
			key1, key2,
			TableStat::getInverseOfReductionFactor({tid1, key1, tid2, key2})
		};
	};

	Operator* load[DEFAULT_SIZE_OF_TABLES + 1] = { nullptr };
	std::priority_queue<JoinOperator::JoinInfo> heap;
	
	auto s = (std::string::size_type)0;
	auto e = command.find("&", s);
	
	while(true)
	{
		auto joinInfo = fetchJoinInfo(command.substr(s, e - s));
		
		if ( load[joinInfo.tid1] == nullptr )
			load[joinInfo.tid1] = new LoadOperator(joinInfo.tid1);
		
		if ( load[joinInfo.tid2] == nullptr )
			load[joinInfo.tid2] = new LoadOperator(joinInfo.tid2);

		heap.push(joinInfo);

		if ( e== std::string::npos )
			break;

		s = e + 1;
		e = command.find("&", s);
	}

	// Query Optimizer
	HashJoinOperator* lastOp;
	while ( !heap.empty() )
	{
		auto joinItem = heap.top();
		auto joinOp = new HashJoinOperator();
		joinOp->setLeft(load[joinItem.tid1]);
		joinOp->setRight(load[joinItem.tid2]);
		joinOp->addJoinInfo(joinItem);
		for ( auto i = 1; i <= DEFAULT_SIZE_OF_TABLES; ++i )
			if ( i != joinItem.tid1 && load[i] == load[joinItem.tid1] || i != joinItem.tid2 && load[i] == load[joinItem.tid2] )
				load[i] = lastOp = joinOp;
		load[joinItem.tid1] = load[joinItem.tid2] = lastOp = joinOp;
		heap.pop();
	}

	op = new SumOperator(lastOp);
}

record_val_t ParseTree::execute()
{
	if ( op != nullptr )
		op->execute();
	return op->getSum();
}

ParseTree::~ParseTree()
{
	delete op;
}

TableStat::Dist TableStat::dist(DEFAULT_SIZE_OF_TABLES);

void TableStat::analyze(Table::ID tableID){
	auto next = HEADER_PAGE_OFFSET;

	auto buf = BufferManager::get_frame(tableID, PGNUM(next));
	auto page = BufferManager::get_page(buf, false);
	next = page->getRootPageOffset();
	BufferManager::put_frame(buf);

	if ( next == HEADER_PAGE_OFFSET )
		return;

	const auto num_col = page->getNumOfColumns();

	buf = BufferManager::get_frame(tableID, PGNUM(next));
	page = BufferManager::get_page(buf, false);

	while ( !page->isLeaf() )
	{
		next = page->getOffset(0);

		BufferManager::put_frame(buf);
		buf = BufferManager::get_frame(tableID, PGNUM(next));
		page = BufferManager::get_page(buf, false);
	}

	// Ready for distribution
	dist[tableID].clear();

	while ( true )
	{
		for ( auto i = 0; i < page->getNumOfKeys(); ++i )
		{
			auto record = std::move(page->getRecord(i, num_col));
			for ( auto colNum = 1; colNum <= num_col; ++colNum )
			{
				const auto& key = record[colNum - 1];
				dist[tableID][colNum][key] += 1;
			}
		}

		next = page->getOffset(DEFAULT_LEAF_ORDER - 1);
		if ( next == HEADER_PAGE_OFFSET )
			break;

		BufferManager::put_frame(buf);
		buf = BufferManager::get_frame(tableID, PGNUM(next));
		page = BufferManager::get_page(buf, false);
	}
	BufferManager::put_frame(buf);
}

std::size_t TableStat::getColumnCount(Table::ID tableID, Table::Col colNum)
{
	return dist[tableID][colNum].size();
}

std::size_t TableStat::getInverseOfReductionFactor(JoinInfo joinInfo)
{
	auto tid1 = *joinInfo.begin();
	auto col1 = *(joinInfo.begin() + 1);
	auto tid2 = *(joinInfo.begin() + 2);
	auto col2 = *(joinInfo.begin() + 3);
	
	if ( !dist[tid1][col1].size() || !dist[tid2][col2].size() )
		return 0;

	auto t1_min = dist[tid1][col1].cbegin()->first;
	auto t1_max = dist[tid1][col1].crbegin()->first;
	auto t2_min = dist[tid2][col2].cbegin()->first;
	auto t2_max = dist[tid2][col2].crbegin()->first;
	if ( joinInfo.size() == 8 )
	{
		t1_min = std::max(t1_min, *(joinInfo.begin() + 4));
		t1_max = std::min(t1_max, *(joinInfo.begin() + 5));
		t2_min = std::max(t2_min, *(joinInfo.begin() + 6));
		t2_max = std::min(t2_max, *(joinInfo.begin() + 7));
	}

	size_t cnt_1 = 0, cnt_2 = 0;
	
	auto counting = [&dist](size_t& cnt, int tid, int col, auto min, auto max) {
		auto it = dist[tid][col].find(min);
		auto last_it = dist[tid][col].find(max);
		do
		{
			cnt += it->second;
			it++;
		}
		while ( it != last_it );
	};

	std::thread t1(counting, std::ref(cnt_1), tid1, col1, t1_min, t1_max);
	std::thread t2(counting, std::ref(cnt_2), tid2, col2, t2_min, t2_max);
	t1.join();
	t2.join();
	return cnt_1 * cnt_2;
}
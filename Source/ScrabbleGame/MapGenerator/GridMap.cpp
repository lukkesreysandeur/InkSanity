#include "GridMap.h"

FGridMap::FGridMap(const uint32_t Rows, const uint32_t Cols, const double Width, const double Height,
                   const double WidthScale, const double HeightScale, const uint32_t NumPaths)
{
	this->Rows = Rows;
	this->Cols = Cols;
	this->BlockWidth = WidthScale * Width / Cols;
	this->BlockHeight = HeightScale * Height / Rows;
	this->RandomNumberGenerator.seed(DefaultSeed());

	CreatePaths(NumPaths);
}

FGridMap::FGridMap(const uint32_t Rows, const uint32_t Cols, const double Width, const double Height,
	const double WidthScale, const double HeightScale, const uint32_t NumPaths, const uint32_t Seed)
{
	this->Rows = Rows;
	this->Cols = Cols;
	this->BlockWidth = WidthScale * Width / Cols;
	this->BlockHeight = HeightScale * Height / Rows;
	this->RandomNumberGenerator.seed(Seed);

	CreatePaths(NumPaths);
}

uint64_t FGridMap::HashBlock(const uint32_t Row, const uint32_t Col)
{
	return static_cast<uint64_t>(Row) << 32 | Col;
}

std::tuple<uint32_t, uint32_t> FGridMap::UnhashBlock(const uint64_t Hash)
{
	uint32_t Row = static_cast<uint32_t>(Hash >> 32);
	uint32_t Col = static_cast<uint32_t>(Hash);
	
	return std::make_tuple(Row, Col);
}

void FGridMap::CreatePaths(const uint32_t NumPaths)
{
	std::uniform_int_distribution<int> StartGenerator(0, Cols - 1);

	for (uint32_t i = 0; i < NumPaths; i++)
	{
		// randomly select a starting node path
		uint32_t CurrentCol = StartGenerator(RandomNumberGenerator);
		uint64_t PrevBlock = HashBlock(0, CurrentCol);
		NodeList.insert(PrevBlock);
		
		for (uint32_t CurrentRow = 1; CurrentRow < Rows - 1; CurrentRow++)
		{
			int LeftBound = -1;
			int RightBound = 1;

			if (CurrentCol == 0)
			{
				LeftBound = 0;
			} else if (std::vector<uint64_t> To = EdgeList[HashBlock(CurrentRow - 1, CurrentCol - 1)];
				std::find(To.begin(), To.end(), HashBlock(CurrentRow, CurrentCol)) != To.end())
			{
				LeftBound = 0;
			}

			if (CurrentCol == Cols - 1)
			{
				RightBound = 0;
			} else if (std::vector<uint64_t> To = EdgeList[HashBlock(CurrentRow - 1, CurrentCol + 1)];
				std::find(To.begin(), To.end(), HashBlock(CurrentRow, CurrentCol)) != To.end())
			{
				RightBound = 0;
			}

			std::uniform_int_distribution<int> NextSquare(LeftBound, RightBound);
			
			CurrentCol += NextSquare(RandomNumberGenerator);

			uint64_t CurrentBlock = HashBlock(CurrentRow, CurrentCol);
			
			NodeList.insert(CurrentBlock);
			if (!EdgeList.contains(PrevBlock))
			{
				EdgeList[PrevBlock] = std::vector<uint64_t>();
			}

			EdgeList.find(PrevBlock)->second.push_back(CurrentBlock);
			
			PrevBlock = CurrentBlock;
		}
	}
}

std::tuple<TArray<FVector2D>, TArray<FVector2D>> FGridMap::GenerateGraph() const 
{
	TArray<FVector2D> Vertices = TArray<FVector2D>();
	TArray<FVector2D> Edges = TArray<FVector2D>();
	const FVector2D BossNode = FVector2D(BlockWidth * Cols * 0.5, BlockHeight * (Rows - 0.5));
	Vertices.Add(BossNode);

	std::random_device RandomDevice;
	std::mt19937 Generator(RandomDevice());

	std::unordered_map<uint64_t, uint32_t> IndexMap = std::unordered_map<uint64_t, uint32_t>();
	
	std::uniform_real_distribution<> XCoordinateGenerator(0.25 * BlockWidth, 0.75 * BlockWidth);
	std::uniform_real_distribution<> YCoordinateGenerator(0.25 * BlockHeight, 0.75 * BlockHeight);
	
	for (const uint64_t NodeHash : NodeList)
	{
		std::tuple<uint32_t, uint32_t> Coords = UnhashBlock(NodeHash);
		const uint32_t Row = std::get<0>(Coords);
		const uint32_t Col = std::get<1>(Coords);

		const double XCoordinate = Col * BlockWidth + XCoordinateGenerator(Generator);
		const double YCoordinate = Row * BlockHeight + YCoordinateGenerator(Generator);

		IndexMap[NodeHash] = Vertices.Num();
		Vertices.Add(FVector2D(XCoordinate, YCoordinate));
	}

	
	for (const auto& [Key, Value] : EdgeList)
	{
		const int FromIndex = IndexMap[Key];
		
		for (const uint64_t Node : Value)
		{
			const int ToIndex = IndexMap[Node];

			Edges.Add(FVector2D(FromIndex, ToIndex));
			
			// connect to boss node
			std::tuple<uint32_t, uint32_t> Coords = UnhashBlock(Node);
			if (const uint32_t Row = std::get<0>(Coords); Row == Rows - 2)
			{
				Edges.Add(FVector2D(ToIndex, 0));
			}
		}
	}

	return std::tuple(Vertices, Edges);
}

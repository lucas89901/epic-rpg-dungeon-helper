#include <cassert>
#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

#define READVAR(x)          \
  std::cout << #x << " = "; \
  std::cin >> x;

namespace dungeon_helper {
unsigned int RandInt() {
  int64_t seed =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::mt19937 generator(seed);
  return generator();
}

enum class Block {
  kSmallWhite = 0,
  kBorderWhite,
  kBlack,
  kBigWhite,
};

enum class DungeonStatus {
  kPassed = 0,
  kDead,
  kRunning,
};

struct Player {
  int r;
  int c;
  int hp;
  int orb_count = 0;
};

class Dungeon {
 public:
  Dungeon(int size, int desired_orb_count, int initial_hp)
      : kSize(size),
        kDesiredOrbCount(desired_orb_count),
        grid_initial_(kSize, std::vector<Block>(kSize)) {
    for (int i = 0; i < kSize; ++i) {
      for (int j = 0; j < kSize; ++j) {
        grid_initial_[i][j] = static_cast<Block>(RandInt() % 4);
      }
    }
    grid_initial_[1][1] = Block::kBigWhite;
    grid_ = grid_initial_;
    player_initial_ = NewPlayer(initial_hp);
    player_ = player_initial_;
  }

  void Set(std::vector<std::vector<int>> grid) {
    assert(grid.size() == kSize);
    assert(grid[0].size() == kSize);
    for (int i = 0; i < kSize; ++i) {
      for (int j = 0; j < kSize; ++j) {
        grid_[i][j] = static_cast<Block>(grid[i][j]);
      }
    }
    assert(grid_[1][1] == Block::kBigWhite);
  }

  void Reset() {
    grid_ = grid_initial_;
    player_ = player_initial_;
    status_ = DungeonStatus::kRunning;
    past_moves_.clear();
  }

  void Print() {
    for (int i = 0; i < kSize; ++i) {
      for (int j = 0; j < kSize; ++j) {
        if (i == player_.r && j == player_.c) {
          std::cout << "(P)";
        }
        std::cout << static_cast<int>(grid_[i][j]) << ' ';
      }
      std::cout << std::endl;
    }

    std::cout << "HP: " << player_.hp << ' ' << "Orbs: " << player_.orb_count
              << std::endl;

    std::cout << "Past moves: ";
    for (char &move : past_moves_) {
      std::cout << move;
    }
    std::cout << std::endl;
  }

  void Move(char direction) {
    std::pair<int, int> next_position = NextPosition(direction);
    int rf = next_position.first;
    int cf = next_position.second;

    if (!IsInGrid(rf, cf)) {
      std::cout << "ERROR - Move out of bound!!" << std::endl;
      return;
    }

    player_.r = rf;
    player_.c = cf;
    UpdateGrid();
    UpdatePlayer();
    past_moves_.push_back(direction);

    // Print();

    if (player_.hp < 0) {
      // std::cout << "YOU DIED!!" << std::endl;
      status_ = DungeonStatus::kDead;
    }
  }

  void Attack() {
    if (PlayerOnBlock() != Block::kBigWhite) {
      std::cout << "ERROR - Not standing on big white!" << std::endl;
      return;
    }
    if (player_.orb_count < 10) {
      // std::cout << "ERROR - Not enough orbs!" << std::endl;
      return;
    }
    std::cout << "Attack successful!" << std::endl;
    status_ = DungeonStatus::kPassed;
  }

  DungeonStatus status() {
    return status_;
  }
  int size() const {
    return kSize;
  }

 private:
  friend class DungeonRunner;

  Player NewPlayer(int hp) {
    Player player;
    player.r = 1;
    player.c = 1;
    player.hp = hp;
    return player;
  }

  inline Block NextBlock(Block &block) {
    return static_cast<Block>((static_cast<int>(block) + 1) % 4);
  }
  inline bool IsInGrid(int row, int column) {
    return 0 <= row && row < grid_.size() && 0 <= column &&
           column < grid_[0].size();
  }
  inline Block PlayerOnBlock() {
    return grid_[player_.r][player_.c];
  }
  std::pair<int, int> NextPosition(char direction) {
    switch (direction) {
      case 'u':
        return {player_.r - 1, player_.c};
      case 'd':
        return {player_.r + 1, player_.c};
      case 'l':
        return {player_.r, player_.c - 1};
      case 'r':
        return {player_.r, player_.c + 1};
    }
  }

  void UpdateGrid() {
    for (int i = 0; i < kSize; ++i) {
      for (int j = 0; j < kSize; ++j) {
        if (i == player_.r && j == player_.c) {
          continue;
        }
        grid_[i][j] = NextBlock(grid_[i][j]);
      }
    }
  }

  void UpdatePlayer() {
    player_.hp -= 30;
    switch (PlayerOnBlock()) {
      case Block::kSmallWhite:
        player_.hp += 5;
        player_.orb_count = std::max(0, player_.orb_count - 5);
        break;
      case Block::kBorderWhite:
        player_.hp -= 500;
        break;
      case Block::kBlack:
        player_.hp -= 25;
        ++player_.orb_count;
        break;
    }
  }

  DungeonStatus status_ = DungeonStatus::kRunning;

  const int kSize;
  const int kDesiredOrbCount;

  std::vector<std::vector<Block>> grid_initial_;
  std::vector<std::vector<Block>> grid_;

  Player player_initial_;
  Player player_;

  std::vector<char> past_moves_;
};

class DungeonRunner {
 public:
  DungeonRunner(Dungeon dungeon) : dungeon_(dungeon) {}

  void Run(int try_count) {
    for (int i = 1; i <= try_count; ++i) {
      if (i % 10000 == 0) {
        std::cout << "Running try number " << i << "..." << std::endl;
      }

      bool found = Try();
      if (found) {
        std::cout << "Solution found: " << MovesToString(dungeon_.past_moves_)
                  << std::endl;
        break;
      } else {
        // std::cout << "Failed try: " << MovesToString(dungeon_.past_moves_) <<
        // std::endl;
      }
    }
  }

 private:
  bool Try() {
    dungeon_.Reset();
    while (dungeon_.status() == DungeonStatus::kRunning) {
      std::vector<char> possible_directions;
      // Find all possible directions for next move.
      for (const char &direction : kDirections) {
        std::pair<int, int> next_position = dungeon_.NextPosition(direction);
        if (dungeon_.IsInGrid(next_position.first, next_position.second)) {
          possible_directions.push_back(direction);
        }
      }

      std::vector<char> ideal_directions;
      for (char &c : possible_directions) {
        std::pair<int, int> next_position = dungeon_.NextPosition(c);
        if (dungeon_.grid_[next_position.first][next_position.second] !=
            Block::kBorderWhite) {
          ideal_directions.push_back(c);
        }
      }
      if (!ideal_directions.empty()) {
        possible_directions = ideal_directions;
      }

      dungeon_.Move(
          possible_directions[RandInt() % possible_directions.size()]);
      if (dungeon_.PlayerOnBlock() == Block::kBigWhite) {
        dungeon_.Attack();
      }
    }
    return dungeon_.status() == DungeonStatus::kPassed;
  }

  std::string MovesToString(std::vector<char> &moves) {
    std::string ret;
    for (size_t i = 0; i < moves.size(); ++i) {
      ret += kDirectionFull.at(moves[i]);
      if (i != moves.size() - 1) {
        ret += " -> ";
      }
    }
    return ret;
  }

  const std::vector<char> kDirections = {'u', 'd', 'l', 'r'};
  const std::map<char, std::string> kDirectionFull = {
      {'u', "Up"}, {'d', "Down"}, {'l', "Left"}, {'r', "Right"}};

  Dungeon dungeon_;
};
}  // namespace dungeon_helper

int main() {
  int initial_hp;
  bool auto_mode;
  READVAR(initial_hp);
  READVAR(auto_mode);
  dungeon_helper::Dungeon dungeon(3, 10, initial_hp);

  // std::vector<std::vector<int>> grid = {{2, 1, 1}, {2, 3, 1}, {3, 1, 1}};
  //  dungeon.Set(grid);

  std::cout << "=== Starting conditions ===" << std::endl;
  dungeon.Print();
  std::cout << "===========================" << std::endl;

  if (auto_mode) {
    // int try_count;
    // READVAR(try_count);
    dungeon_helper::DungeonRunner runner(dungeon);
    runner.Run(10000000);
  } else {
    while (dungeon.status() == dungeon_helper::DungeonStatus::kRunning) {
      char op;
      READVAR(op);
      switch (op) {
        case 'u':
        case 'd':
        case 'l':
        case 'r':
          dungeon.Move(op);
          break;
        case 'a':
          dungeon.Attack();
          break;
        default:
          std::cout << "ERROR - Unknown operation" << std::endl;
      }
      dungeon.Print();
    }
  }

  return 0;
}

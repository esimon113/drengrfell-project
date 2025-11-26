#include "player.h"
#include "types.h"

#include <algorithm>


namespace df{

    Player::Player()
        : playerId(0), heroPoints(0), heroReference(nullptr)
    {
    }

    Player::Player(size_t id)
        : playerId(id), heroPoints(0), heroReference(nullptr)
    {
    }

    size_t Player::getId() const{
        return this->playerId;
    }

    int Player::getHeroPoints() const{
        return heroPoints;
    }

    void Player::addHeroPoints(int points){
        heroPoints += points;
    }

    void Player::setHeroPoints(int points){
        heroPoints = points;
    }

    const std::vector<Settlement *> &Player::getSettlement() const
    {
        return  settlements;
    }

    void Player::addSettlement(Settlement *settlement){
        settlements.push_back(settlement);
    }

    void Player::removeSettlement(Settlement *settlement){
        if(settlement == nullptr) return;
        // Could the user have the possibillity of removing a settlement that he does not own? If yes we should implement a check here
        settlements.erase(std::remove(settlements.begin(), settlements.end(), settlement), settlements.end());
    }

    void Player::addResources(types::TileType type, int amount){
        resources[type] += amount;
    }

    void Player::removeResources(types::TileType type, int amount){
        // no check is we call first the hasResources function
        resources[type] -= amount;
    }

    int Player::getResources(types::TileType type) const{
        auto it = resources.find(type);
        if(it != resources.end()){
            return it->second;
        }
        return 0;
    }

    bool Player::hasResources(const std::map<types::TileType, int>& amountRequired){
        for(const auto& [type,amount ]: amountRequired){
            if(getResources(type) < amount){
                return false;
            } 
        }
        return true;
    }

    const std::map<types::TileType, int> &Player::getResources() const{
        return resources;
    }

    void Player::setHero(Hero *hero){
        heroReference = hero;
    }

    Hero *Player::getHero() const{
        return heroReference;
    }

    void Player::addRoad(Road *road){
        roads.push_back(road);
    }

    const std::vector<Road *> &Player::getRoads() const{
        return roads;
    }

    int Player::getRoadCount() const{
        return roads.size();
    }

    void Player::exploreTile(Tile *tile){
        if(tile == nullptr) return;
        if(!isTileExplored(tile)){
            exploredTiles.push_back(tile);
        }
        
    }

    bool Player::isTileExplored(const Tile *tile) const{
        if(tile == nullptr) return false;
        return std::find(exploredTiles.begin(), exploredTiles.end(), tile) != exploredTiles.end();
    }

    bool Player::isTileExplored(size_t tileId) const{ 
        for(const auto& tile : exploredTiles){
            if(tile != nullptr && tile->getId() == tileId){
                return true;
            }
        }
        return false;
    }

    const std::vector<Tile *> &Player::getExploredTiles() const{
        return exploredTiles;
    }

    void Player::reset(){
        heroPoints = 0;
        settlements.clear();
        resources.clear();
        heroReference = nullptr;
        roads.clear();
        exploredTiles.clear();
    }

    size_t Player::getPlayerId() const { return playerId; }
    void Player::setPlayerId(size_t newPlayerId) { playerId = newPlayerId; }

    const json Player::serialize() const {
        /* TODO: dont know how to store references in json.we should use registry for data instead
        this data left:
        Hero* heroReference;
        std::vector<Road*> roads;
        std::vector<Tile*> exploredTiles;
        std::vector<Settlement*> settlements;
        std::map<types::TileType, int> resources;
        */
        json j;

        j["playerId"] = playerId;
        j["heroPoints"] = heroPoints;
        j["settlementCount"] = settlementCount;

        return j;
    }

    void Player::deserialize(const json& j) {
        // TODO: deserialize from json. see comment on serialize
        this->setPlayerId(j.at("playerId").get<size_t>());
        this->setHeroPoints(j.at("heroPoints").get<int>());
        this->settlementCount = (j.at("settlementCount").get<int>());
    }

}


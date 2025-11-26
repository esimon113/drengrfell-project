#include "player.h"
#include "types.h"

#include <algorithm>


namespace df{

    Player::Player(): playerId(0), playerPoints(0), heroReference(0){}
    Player::Player(size_t id): playerId(id), playerPoints(0), heroReference(0){}

    size_t Player::getId() const{return this->playerId;}

    
    int Player::getPlayerPoints() const{return playerPoints;}
    void Player::addPlayerPoints(int points ){playerPoints += points;}
    void Player::setPlayerPoints(int points){playerPoints = points;}
    
    // HERO 

    void Player::setHero(size_t hero){heroReference = hero;}
    size_t Player::getHero() const{return heroReference;}

    

    // SETTLEMENTS

    const std::vector<size_t> &Player::getSettlement() const{return  settlements;} // change settlement to settlements 

    void Player::addSettlement(size_t settlement){
        if(std::find(settlements.begin(), settlements.end(), settlement) == settlements.end()){
            settlements.push_back(settlement);
        }
    }

    // RESOURCES

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
    
    // ROADS

    void Player::addRoad(size_t road){
        if(std::find(roads.begin(), roads.end(), road) == roads.end()){
            roads.push_back(road);
        }
    }

    const std::vector<size_t> &Player::getRoads() const{
        return roads;
    }

    // EXPLORED TILES

    void Player::exploreTile(size_t tile){
        if(!isTileExplored(tile)){
            exploredTiles.push_back(tile);
        }
        
    }

    bool Player::isTileExplored(size_t tileId) const{ 
        return std::find(exploredTiles.begin(), exploredTiles.end(), tileId) != exploredTiles.end();
    }

    const std::vector<size_t> &Player::getExploredTiles() const{
        return exploredTiles;
    }

    // RESET

    void Player::reset(){
        playerPoints = 0;
        settlements.clear();
        resources.clear();
        heroReference = 0;
        roads.clear();
        exploredTiles.clear();
    }

    

    const json Player::serialize() const {
        json j;

        j["playerId"] = playerId;
        j["playerPoints"] = playerPoints;
        j["settlements"] = settlements;

        json resourcesJson = json::object();
        for (const auto& [type, amount] : resources) {
            resourcesJson[std::to_string(static_cast<int>(type))] = amount;
        }
        j["resources"] = resourcesJson;

        j["heroReference"] = heroReference;
        j["roads"] = roads;
        j["exploredTiles"] = exploredTiles;

        return j;
    }

    void Player::deserialize(const json& j) {

        playerId = j.at("playerId").get<size_t>();
        playerPoints = j.at("playerPoints").get<int>();
        settlements = j.at("settlements").get<std::vector<size_t>>();

        resources.clear();
        if(j.contains("resources")){
            for (auto& [key, value] : j.at("resources").items()) {
                types::TileType type = static_cast<types::TileType>(std::stoi(key));
                int amount = value.get<int>();
                resources[type] = amount;
            }
        }
        if(j.contains("heroReference")){
            heroReference = j.at("heroReference").get<size_t>();
        }

        if(j.contains("roads")){
            roads = j.at("roads").get<std::vector<size_t>>();
        }

        if(j.contains("exploredTiles")){
            exploredTiles = j.at("exploredTiles").get<std::vector<size_t>>();
        }
        
    }

}


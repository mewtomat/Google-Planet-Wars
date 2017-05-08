// This file contains helper code that does all the boring stuff for you.
// The code in this file takes care of storing lists of planets and fleets, as
// well as communicating with the game engine. You can get along just fine
// without ever looking at this file. However, you are welcome to modify it
// if you want to.
#ifndef PLANET_WARS_H_
#define PLANET_WARS_H_

#include <string>
#include <vector>
#include <fstream>
#include <map>
// #include "Commons.h"

extern std::vector<std::vector<int> > distance_;
extern int flip(int a);
// This is a utility class that parses strings.
class StringUtil {
 public:
  // Tokenizes a string s into tokens. Tokens are delimited by any of the
  // characters in delimiters. Blank tokens are omitted.
  static void Tokenize(const std::string& s,
                       const std::string& delimiters,
                       std::vector<std::string>& tokens);

  // A more convenient way of calling the Tokenize() method.
  static std::vector<std::string> Tokenize(
                       const std::string& s,
                       const std::string& delimiters = std::string(" "));
};

// This class stores details about one fleet. There is one of these classes
// for each fleet that is in flight at any given time.
class Fleet {
 public:
  // Initializes a fleet.
  Fleet(int owner,
        int source_planet = -1,
        int destination_planet = -1,
        int num_ships=0,
        // int total_trip_length = -1,
        // int turns_remaining = -1,
        int launch = 0);

  // Returns the playerID of the owner of the fleet. Your player ID is always
  // 1. So if the owner is 1, you own the fleet. If the owner is 2 or some
  // other number, then this fleet belongs to your enemy.
  int Owner() const;

  // Returns the number of ships that comprise this fleet.
  int NumShips() const;

  // Returns the ID of the planet where this fleet originated.
  int SourcePlanet() const;

  // Returns the ID of the planet where this fleet is headed.
  int DestinationPlanet() const;

  // Returns the total distance that is being traveled by this fleet. This
  // is the distance between the source planet and the destination planet,
  // rounded up to the nearest whole number.
  // int TotalTripLength() const;

  // Returns the number of turns until this fleet reaches its destination. If
  // this value is 1, then the fleet will hit the destination planet next turn.
  // int TurnsRemaining() const;

  int Remaining() const;
  int Length() const;
  int Launch() const;
  int launch_;

 private:
  int owner_;
  int num_ships_;
  int source_planet_;
  int destination_planet_;
  // int total_trip_length_;
  // int turns_remaining_;
};

class FleetSummary
{
private:

public:
  int enemy_;
  int ally_;
  int& operator[](int i)
  {
    if(i==1) return ally_;
    else return enemy_;
  }
  int operator()(int i)const{
    if(i==1) return ally_;
    else return enemy_;
  }

  int delta(int player) const
  {
    if(player == 1)
    {
      return ally_- enemy_;
    }
    else return enemy_ - ally_;
  }

  bool empty() const
  {return (enemy_ == 0 && ally_==0);}
};

class PlanetState
{
public:
  int owner;
  int ships;
};
// Stores information about one planet. There is one instance of this class
// for each planet on the map.
class Planet {
 public:
  // Initializes a planet.
  Planet(int planet_id,
         int owner,
         int num_ships,
         int growth_rate,
         double x,
         double y,
         bool prediction);
Planet();
  // Returns the ID of this planets. Planets are numbered starting at zero.
  int PlanetID() const;

  // Returns the ID of the player that owns this planet. Your playerID is
  // always 1. If the owner is 1, this is your planet. If the owner is 0, then
  // the planet is neutral. If the owner is 2 or some other number, then this
  // planet belongs to the enemy.
  int Owner() const;
  int Ships(bool locked=false) const;
  // The number of ships on the planet. This is the "population" of the planet.
  int NumShips() const;

  // Returns the growth rate of the planet. Unless the planet is neutral, the
  // population of the planet grows by this amount each turn. The higher this
  // number is, the faster this planet produces ships.
  int GrowthRate() const;

  // The position of the planet in space.
  double X() const;
  double Y() const;

  // Use the following functions to set the properties of this planet. Note
  // that these functions only affect your program's copy of the game state.
  // You can't steal your opponent's planets just by changing the owner to 1
  // using the Owner(int) function! :-)
  void Owner(int new_owner);
  void NumShips(int new_num_ships);
  void AddShips(int amount);
  int RemoveShips(int amount);
  std::vector<FleetSummary> IncomingFleets() const;
  void AddIncomingFleet(Fleet f);
  PlanetState FutureState(unsigned int days) ;
  int FutureOwner() ;
  int LockShips( int ships );
  int LockedShips(); 
  void UnLockShips(int ships); 
  int FutureDays() ;
  void UpdatePrediction() ;
  int Cost(unsigned int days, int player) ;
  int RequiredShips() const;    
  int ShipExcess(unsigned int days) const;
 private:
  bool update_prediction_;
  int planet_id_;
  int owner_;
  int num_ships_;
  int growth_rate_;
  double x_, y_;
  int locked_ships_;
  std::vector<FleetSummary> incoming_fleets_;
  std::vector<PlanetState> prediction_;
  // std::vector<>
};

class PlanetWars {
 public:
  // Initializes the game state given a string containing game state data.
  PlanetWars(const std::string& game_state);

  // Returns the number of planets on the map. Planets are numbered starting
  // with 0.
  int NumPlanets() const;

  // Returns the planet with the given planet_id. There are NumPlanets()
  // planets. They are numbered starting at 0.
  Planet& GetPlanet(int planet_id) ;
  const Planet& GetPlanet(int planet_id) const;

  // Returns the number of fleets.
  int NumFleets() const;

  // Returns the fleet with the given fleet_id. Fleets are numbered starting
  // with 0. There are NumFleets() fleets. fleet_id's are not consistent from
  // one turn to the next.
  const Fleet& GetFleet(int fleet_id) const;

  // Returns a list of all the planets.
  std::vector<Planet> Planets() const;

  // Return a list of all the planets owned by the current player. By
  // convention, the current player is always player number 1.
  std::vector<Planet> MyPlanets() const;

  // Return a list of all neutral planets.
  std::vector<Planet> NeutralPlanets() const;

  // Return a list of all the planets owned by rival players. This excludes
  // planets owned by the current player, as well as neutral planets.
  std::vector<Planet> EnemyPlanets() const;

  // Return a list of all the planets that are not owned by the current
  // player. This includes all enemy planets and neutral planets.
  std::vector<Planet> NotMyPlanets() const;

  // Return a list of all the fleets.
  std::vector<Fleet> Fleets() const;

  // Return a list of all the fleets owned by the current player.
  std::vector<Fleet> MyFleets() const;

  // Return a list of all the fleets owned by enemy players.
  std::vector<Fleet> EnemyFleets() const;

  // Writes a string which represents the current game state. This string
  // conforms to the Point-in-Time format from the project Wiki.
  std::string ToString() const;

  // Returns the distance between two planets, rounded up to the next highest
  // integer. This is the number of discrete time steps it takes to get between
  // the two planets.
  int Distance(int source_planet, int destination_planet) const;

  // Sends an order to the game engine. The order is to send num_ships ships
  // from source_planet to destination_planet. The order must be valid, or
  // else your bot will get kicked and lose the game. For example, you must own
  // source_planet, and you can't send more ships than you actually have on
  // that planet.
  void AddFleet(const Fleet& order) ;
  void PlaceOrder(const Fleet& f);
  // void PlaceOrder(int source, int dest, int num_ships) ;

  std::vector<Fleet> Orders() const;


  void IssueOrder(int source_planet,
		  int destination_planet,
		  int num_ships) const;

  // Returns true if the named player owns at least one planet or fleet.
  // Otherwise, the player is deemed to be dead and false is returned.
  bool IsAlive(int player_id) const;

  // Returns the number of ships that the given player has, either located
  // on planets or in flight.
  int NumShips(int player_id) const;

  // Returns the total growth rate of the planets that the given player has
  int Production(int player_id) const;
	
  // Sends a message to the game engine letting it know that you're done
  // issuing orders for now.
  void FinishTurn() const;

  void Init();
  std::vector<int> PlanetsOwnedBy(int player) const;

  int ClosestPlanetByPlayer(int planet , int player) const;


  std::vector<int> PlanetsByDistance(int planet_id) const;

  void Init() const;

  int ShipsWithinRange(int, int , int)const;
  std::map<int,bool> FrontierPlanets(int player) const;
  std::map<int,bool> FutureFrontierPlanets(int player);
  std::vector<int> FrontierPlanetsVector(int player) const ;
  std::vector<int> NotFrontierPlanets(int player) const;
  double Potential(int planet_id, int player);
 
 private:

  // Parses a game state from a string. On success, returns 1. On failure,
  // returns 0.
  int ParseGameState(const std::string& s);

  // Store all the planets and fleets. OMG we wouldn't wanna lose all the
  // planets and fleets, would we!?
  std::vector<Planet> planets_;
  std::vector<Fleet> fleets_;
  std::vector<std::vector<int> > by_distance_;
  std::vector<Fleet> orders_;

};



#endif

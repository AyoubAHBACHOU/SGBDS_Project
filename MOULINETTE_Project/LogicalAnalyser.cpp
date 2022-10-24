#include "LogicalAnalyser.h"
using namespace std;

LogicalAnalyser *LogicalAnalyser::logicalAnalyser_ = nullptr;
LogicalAnalyser::LogicalAnalyser(const string param_file,
                                 const string solution_file,
                                 const string datafile_file)
{
    this->datafile_in.open(datafile_file);
    this->solution_in.open(solution_file);
    this->param_in.open(param_file);

    if (!(this->datafile_in.is_open() && this->solution_in.is_open() && this->param_in.is_open()))
    {
        cerr << "Sorry we can't open one or more required files : " << datafile_file << ", " << param_file << ", " << solution_file << endl;
        exit(-1);
    }
    constructTripsPopulation();
}

LogicalAnalyser *LogicalAnalyser::getInstance(const string param_file,
                                              const string solution_file,
                                              const string datafile_file)
{
    if (logicalAnalyser_ == nullptr)
    {
        logicalAnalyser_ = new LogicalAnalyser(param_file, solution_file, datafile_file);
    }
    return logicalAnalyser_;
}

// Business Traitement
void LogicalAnalyser::constructTripsPopulation()
{
    string line;

    while (getline(this->datafile_in, line))
    {
        if (line.find("{") != string::npos)
        {
            // CONSTRUCT BUS STATIONS
            if (line.find("BusStations") != string::npos)
            {
                busStations = handle_file_stream_bus_stations(this->datafile_in);
            }

            // CONSTRUCT BUS TRIPS
            else if (line.find("BusTrip") != string::npos)
            {
                handle_file_stream_bus_trips(this->tripsStations, this->datafile_in, busStations);
            }

            // CONSTRUCT INTER TRIPS
            else if (line.find("InterTrips") != string::npos)
            {
                handle_file_stream_inter_trips(interTrips, this->datafile_in, busStations);
            }
        }
        else if (line.find("}") != string::npos)
        {
            // std::cout << "Op. ends at " << line << endl;
        }
    }

    // DETACH LINES AND HAVE ONE SET OF TRIPS POPULATION
    detach_lines(this->tripsPopulation, tripsStations);
}

const bool LogicalAnalyser::findInterTripByTripId(BusTrip &temp, string tripId) const
{

    // we need to find more compatible
    for (auto trip : this->tripsPopulation)
    {
        if (trip.getTripId() == tripId)
        {
            temp = trip;
            return true;
        }
    }

    return false;
}

void LogicalAnalyser::showBusTripsPopulation()
{
    for (auto v : this->tripsPopulation)
    {
        v.showBusTrip();
    }
}

bool LogicalAnalyser::verifyTwoTrips(string T_1, string T_2)
{
    //
}

void LogicalAnalyser::rulesVerfication(const vector<vector<string>> &clusters) const
{
    // Trips Number
    int TRIPS_COUNT = this->tripsPopulation.size();

    // trip existance
    map<string, int> existance;

    //

    // clusters iterator
    vector<vector<string>>::iterator c_it;

    // data iterator
    vector<string>::iterator d_it;
    int cluster_count = 0;
    for (auto cluster : clusters)
    {
        cluster_count++;

        BusTrip startTrip;
        bool found = this->findInterTripByTripId(startTrip, cluster[0]);
        if (found)
        {
            BusTrip currentTrip;
            string between;

            d_it = cluster.begin();
            ++d_it;
            // the value could have two different states ; Trip || (HLP or WS)
            for (; d_it != cluster.end(); ++d_it)
            {
                if ((*d_it) == "HLP")
                {
                    between = "HLP";
                }
                else if ((*d_it) == "WS")
                {
                    between = "WS";
                }
                // Trip handling
                else
                {
                    bool found2 = this->findInterTripByTripId(currentTrip, (*d_it));
                    if (found2)
                    {
                        // traitement starts
                        if (startTrip < currentTrip) // are successor
                        {
                            // the two trips have the same Arrival and Departure station
                            if (startTrip.getBusStationArr()->getId() == currentTrip.getBusStationDep()->getId())
                            {
                                double diff_time = difftime(currentTrip.getDateDep(), startTrip.getDateArr()) / 60;
                                // SI ON PEUT TOLERER ATTENTE = 0 DANS UNE STATION
                                if (0 <= diff_time && diff_time < 45) // the successor could be the next
                                {

                                    

                                    {
                                        if (between != "WS") // between needs to be WS
                                        {
                                            // NOT OK
                                            cerr << "The state between " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " needs to be WS instead of " << between << endl;
                                        }
                                    }
                                }
                                else if (diff_time >= 45)
                                {
                                    cerr << "Trips " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " couldn't be linked -> because difftime passes the 45 min " << endl;
                                }
                                else
                                {
                                    cerr << "The difftime between " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " is negative so trips can't be reached " << endl;
                                }
                            }
                            else // when the stations are different
                            {
                                // search inter trip between stations
                                int temp_dur = TargetInterTrip::findDurationByTargetId(currentTrip.getBusStationDep()->getId(), this->interTrips.at(startTrip.getBusStationArr()->getId()));

                                if (temp_dur != -1) // inter trip exists
                                {

                                    int attente_hlp = (difftime(currentTrip.getDateDep(), startTrip.getDateArr()) / 60.0) - temp_dur;

                                    if (0 <= attente_hlp && attente_hlp < 45)
                                    {
                                        // verifier si HLP
                                        if (between != "HLP")
                                        {
                                            cerr << "The state between " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " needs to be HLP instead of " << between << endl;
                                        }
                                    }
                                    else if (attente_hlp < 0)
                                    {
                                        cout << "dep: " << currentTrip.getDateDep() << " ARR: " << startTrip.getDateArr() << " - " << temp_dur << " att: " << attente_hlp << endl;

                                        cerr << "Le bus ne peut pas achever le Trip " << startTrip.getTripId() << " => " << currentTrip.getTripId() << " au temps " << endl;
                                    }
                                    else
                                    {
                                        cout << "dep: " << currentTrip.getDateDep() << " ARR: " << startTrip.getDateArr() << " - " << temp_dur << " att: " << attente_hlp << endl;
                                        cerr << "L'attente dans la station d'arrivee est > a 45 " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " Le vehicule doit se deplacer vers le depot " << endl;
                                    }
                                }
                                else
                                    cerr << "il y a pas d'intertrip entre " << startTrip.getBusStationArr()->getId() << " and " << currentTrip.getBusStationDep()->getId() << " Trips: " << startTrip.getTripId() << " -> " << currentTrip.getTripId() << endl;
                            }
                        }
                        else
                        {
                            cerr << "The trips " << startTrip.getTripId() << " and " << currentTrip.getTripId() << " are not even succesors by start date " << endl;
                        }
                    }
                    // the destination trip not found
                    else
                    {
                        cerr << "The trip :  " << (*d_it) << " not even exist in data set " << endl;
                        exit(-1);
                    }

                    // next start will be the current
                    startTrip = currentTrip;
                }
            }
        }
        else
        {
            cerr << "Sorry we can't find the trip " << cluster[0] << " on the cluster : " << cluster_count << endl;
        }
    }
}
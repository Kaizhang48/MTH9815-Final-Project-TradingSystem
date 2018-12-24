/**
 * soa.hpp
 * Definition of our Service Oriented Architecture (SOA) Service base class
 *
 * @author Breman Thuraisingham
 */

#ifndef SOA_HPP
#define SOA_HPP

#include <vector>
#include <string>
#include "boost/algorithm/string.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"

using namespace std;

/**
 * Definition of a generic base class ServiceListener to listen to add, update, and remve
 * events on a Service. This listener should be registered on a Service for the Service
 * to notify all listeners for these events.
 */
template<typename V>
class ServiceListener
{

public:

  // Listener callback to process an add event to the Service
  virtual void ProcessAdd(V &data) = 0;

  // Listener callback to process a remove event to the Service
  virtual void ProcessRemove(V &data) = 0;

  // Listener callback to process an update event to the Service
  virtual void ProcessUpdate(V &data) = 0;

};

/**
 * Definition of a generic base class Service.
 * Uses key generic type K and value generic type V.
 */
template<typename K, typename V>
class Service
{

public:

  // Get data on our service given a key
  virtual V& GetData(K key) = 0;

  // The callback that a Connector should invoke for any new or updated data
  virtual void OnMessage(V &data) = 0;

  // Add a listener to the Service for callbacks on add, remove, and update events
  // for data to the Service.
  virtual void AddListener(ServiceListener<V> *listener) = 0;

  // Get all listeners on the Service.
  virtual const vector< ServiceListener<V>* >& GetListeners() const = 0;

};  

/**
 * Definition of a Connector class.
 * This will invoke the Service.OnMessage() method for subscriber Connectors
 * to push data to the Service.
 * Services can invoke the Publish() method on this Service to publish data to the Connector
 * for a publisher Connector.
 * Note that a Connector can be publisher-only, subscriber-only, or both publisher and susbcriber.
 */
template<typename V>
class Connector
{

public:

  // Publish data to the Connector
  virtual void Publish(V &data) = 0;

};

//========= function needed to process the data==========
double StrtoPrice(const std::string& priceStr)
{
	int integer, decimal1, decimal2; // the integer part, xy part and z part

	// split the input string
	std::vector<std::string> stringVec;
	boost::algorithm::split(stringVec, priceStr, boost::algorithm::is_any_of("-"));

	// extract the three parts

	integer = std::stoi(stringVec[0]);
	decimal1 = std::stoi(stringVec[1].substr(0, 2));
	if (stringVec[1].substr(2, 1) == "+")
		decimal2 = 4;
	else
		decimal2 = std::stoi(stringVec[1].substr(2, 1));

	// generate the price
	double price = integer + decimal1 / 32.0 + decimal2 / 256.0;

	return price;
}

// convert price to the price string
// Output format is like 99-xyz where xy is from 00 to 31 and z is from 0 to 7 (z='+' if z=4)

std::string PricetoStr(const double& price)
{
	// the integer part, xy part and z part
	int integer = std::floor(price);
	int	decimal1 = std::floor((price - integer) * 32);
	int decimal2 = (price - integer) * 256 - decimal1 * 8;

	// make the output
	std::string output = std::to_string(integer) + "-";
	if (decimal1 < 10)
		output += "0";
	output += std::to_string(decimal1);
	if (decimal2 == 4)
		output += "+";
	else output += std::to_string(decimal2);

	return output;

}

// convert boost::gregorian::date to us date string
template <typename Date>
std::string DatetoStr(Date d)
{
	std::string output;
	if (d.month().as_number() < 10)
		output += "0";
	output = output + std::to_string(d.month().as_number()) + "/";
	if (d.day() < 10)
		output += "0";
	output = output + std::to_string(d.day()) + "/" + std::to_string(d.year());

	return output;
}
#endif

﻿// Author: Kai Zhang
// @12/23/2018
//Test code fot MTH 9815 Final Project TradingSystem


#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "Parent/soa.hpp"
#include "Parent/products.hpp"
#include "Parent/Timer.hpp"
#include "Parent/productservice.hpp"
#include "DataGenerator/BondPriceDataGenerator.hpp"
#include "DataGenerator/BondInquiryDataGenerator.hpp"
#include "DataGenerator/BondTradeDataGenerator.hpp"
#include "DataGenerator/BondMarketDataGenerator.hpp"
#include "BondService/BondAlgoExecution.hpp"
#include "BondService/BondAlgoStreaming.hpp"
#include "BondService/BondExecution.hpp"
#include "BondService/BondGUI.hpp"
#include "BondService/BondMarketData.hpp"
#include "BondService/BondInquiry.hpp"
#include "BondService/BondPricing.hpp"
#include "BondService/BondRisk.hpp"
#include "BondService/BondStreaming.hpp"
#include "BondService/BondTradeBooking.hpp"
#include "BondService/BondPosition.hpp"
#include "HistoricalDataService/BondInquiryHistoricalDataService.hpp"
#include "HistoricalDataService/BondPositionHistoricalDataService.hpp"
#include "HistoricalDataService/BondRiskHistoricalDataService.hpp"
#include "HistoricalDataService/BondExecutionHistoricalDataService.hpp"
#include "HistoricalDataService/BondStreamingHistoricalDataService.hpp"

int main()
{
	std::cout << "=================== Preparatory Work ========================"<<endl;

	// define the path of the files (may differ between Windows and Unix)
	// input files
	std::string iTradePath("./Data/trades.txt");
	std::string iPricePath("./Data/prices.txt");
	std::string iMarketdataPath("./Data/marketdata.txt");
	std::string iInquiryPath("./Data/inquiries.txt");

	// output files
	std::string oPositionPath("./Data/position.txt");
	std::string oRiskPath("./Data/risk.txt");
	std::string oStreamPath("./Data/streaming.txt");
	std::string oGUIPath("./Data/gui.txt");
	std::string oExecutionPath("./Data/execution.txt");
	std::string oInquiryPath("./Data/allinquiries.txt");

	// product information (hard-coded) 
	//!!要更新到最新
	Bond treasury2Y("9128285M8", CUSIP, "T", 2.750,boost::gregorian::date(2020,Nov,30)); // 2Y bond

	Bond treasury3Y("9128285P1", CUSIP, "T", 2.750,boost::gregorian::date(2021,Nov,30)); // 3Y bond

	Bond treasury5Y("9128285R7", CUSIP, "T", 2.880,boost::gregorian::date(2023,Nov,30)); // 5Y bond

	Bond treasury7Y("9128285N6", CUSIP, "T", 2.880,boost::gregorian::date(2025,Nov,30)); // 7Y bond

	Bond treasury10Y("9128285M8", CUSIP, "T", 3.130,boost::gregorian::date(2028,Nov,30)); // 10Y bond

	Bond treasury30Y("9128285B5", CUSIP, "T", 3.380,boost::gregorian::date(2048,Nov,30)); // 30Y bond

	// bond product service
	BondProductService bondProductService;
	bondProductService.Add(treasury2Y);
	bondProductService.Add(treasury3Y);
	bondProductService.Add(treasury5Y);
	bondProductService.Add(treasury7Y);
	bondProductService.Add(treasury10Y);
	bondProductService.Add(treasury30Y);

	// pv01 information
	//!!需要更新
	std::unordered_map<string,double> pv01Treasury;
	pv01Treasury.insert(std::make_pair(treasury2Y.GetProductId(), 0.0185));
	pv01Treasury.insert(std::make_pair(treasury3Y.GetProductId(), 0.01034));
	pv01Treasury.insert(std::make_pair(treasury5Y.GetProductId(), 0.0176));
	pv01Treasury.insert(std::make_pair(treasury7Y.GetProductId(), 0.02215));
	pv01Treasury.insert(std::make_pair(treasury10Y.GetProductId(), 0.0202));
	pv01Treasury.insert(std::make_pair(treasury30Y.GetProductId(), 0.0275));

	// bucketed sector information
	std::vector<std::string> frontEnd{ treasury2Y.GetProductId() , treasury3Y.GetProductId() }; // front end
	std::vector<std::string> belly{ treasury5Y.GetProductId() , treasury7Y.GetProductId(), treasury10Y.GetProductId() }; // belly
	std::vector<std::string> longEnd{ treasury30Y.GetProductId() }; // long end

	//build BucketTreasury
	std::unordered_map<std::string, std::vector<std::string>> bucketTreasury;
	bucketTreasury.insert(std::make_pair("FrontEnd", frontEnd));
	bucketTreasury.insert(std::make_pair("Belly", belly));
	bucketTreasury.insert(std::make_pair("LongEnd", longEnd));

	// define a timer to record the time
	Timer tm;

	std::cout << "====================================================="<<endl;

	std::cout << "=================== Generate data ========================\n"<<endl;

	//generate data
	tm.Start();
	bond_trade_generator(iTradePath, &bondProductService, "T"); // trade.txt	
	tm.Stop();
	std::cout << "Time spent for trade.txt: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	tm.Start();
	bond_price_generator(iPricePath, &bondProductService, "T"); // price.txt
	tm.Stop();
	std::cout << "Time spent for price.txt: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	tm.Start();
	bond_market_data_generator(iMarketdataPath, &bondProductService, "T"); // marketdata.txt
	tm.Stop();
	std::cout << "Time spent for marketdata.txt: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	tm.Start();
	bond_inquiry_generator(iInquiryPath, &bondProductService, "T"); // inquiry.txt
	tm.Stop();
	std::cout << "Time spent for inquiry.txt: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	std::cout << "==============================================================\n"<<endl;

	std::cout << "=================== Run services ========================" << endl;

	std::cout << "trade.txt	==> position.txt and risk.txt" << endl;
	std::cout << "Data flow: " << endl;
	std::cout << "BondTradeBookingService ==> BondPositionService ==> BondPositionHistoricalDataService" << endl;
	std::cout << "BondTradeBookingService ==> BondPositionService ==> BondRiskService ==> bondRiskHistoricalDataService\n" << endl;

	//// build service components
	BondTradeBookingService bondTradeBookingService;
	BondPositionService bondPositionService(&bondProductService, "T");
 	BondRiskService bondRiskService(&bondProductService, pv01Treasury);
	BondRiskHistoricalDataConnector risktoHistoricalDataConnector(oRiskPath);
	BondRiskHistoricalDataService bondRiskHistoricalDataService(&risktoHistoricalDataConnector);
	BondPositionHistoricalDataConnector positiontoHistoricalDataConnector(oPositionPath);
	BondPositionHistoricalDataService bondPositionHistoricalDataService(&positiontoHistoricalDataConnector);
	
	//build listener
	ToBondPositionListener tradeBookingtoPositionListener(&bondPositionService);
	BondRiskListener positiontoRiskListener(&bondRiskService);
	ToBondRiskHistoricalDataListener risktoHistoricalDataListener(&bondProductService,&bondRiskHistoricalDataService, &bondRiskService, bucketTreasury);
	ToBondPositionHistoricalDataListener positiontoHistoricalDataListener(&bondPositionHistoricalDataService);

	// link the service components
	bondTradeBookingService.AddListener(&tradeBookingtoPositionListener);
	bondPositionService.AddListener(&positiontoRiskListener);
	bondPositionService.AddListener(&positiontoHistoricalDataListener);
	bondRiskService.AddListener(&risktoHistoricalDataListener);

	//start
	tm.Start();
	BondTradeBookingConnector bondTradeBookingConnector(iTradePath, &bondTradeBookingService, &bondProductService);
	tm.Stop();
	std::cout << "Time elapse: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	std::cout << "marketdata.txt ==> execution.txt, position.txt and risk.txt" << endl;
	std::cout << "Data flow: " << endl;
	std::cout << "BondMarketDataService ==> BondAlgoExecutionService ==> BondExecutionService ==> bondExecutionHistoricalDataService\n" << endl;

	// build service components
	BondMarketDataService bondMarketDataService;
	BondAlgoExecutionService bondAlgoExecutionService;
	BondAlgoExecutionListener bondAlgoExecutionListener(&bondAlgoExecutionService);
	BondExecutionService bondExecutionService;
	BondExecutionListener bondExecutionListener(&bondExecutionService);
	ToBondTradeBookingListener bondTradeBookingListener(&bondTradeBookingService);
	BondExecutionHistoricalDataConnector bondExecutionHistoricalDataConnector(oExecutionPath);
	BondExecutionHistoricalDataService bondExecutionHistoricalDataService(&bondExecutionHistoricalDataConnector);
	BondExecutionHistoricalDataListener bondExecutionHistoricalDataListener(&bondExecutionHistoricalDataService);

	// link the service components
	bondMarketDataService.AddListener(&bondAlgoExecutionListener);
	bondAlgoExecutionService.AddListener(&bondExecutionListener);
	bondExecutionService.AddListener(&bondTradeBookingListener);
	bondExecutionService.AddListener(&bondExecutionHistoricalDataListener);

	// start
	tm.Start();
	BondMarketDataConnector bondMarketDataConnector(iMarketdataPath, &bondMarketDataService, &bondProductService);
	tm.Stop();
	std::cout << "Time spent: " << tm.GetTime() << " seconds\n" << endl;
	tm.Reset();

	std::cout << "price.txt ==> streaming.txt and gui.txt"<<endl;
	std::cout << "Data flow: " << endl;
	std::cout << "BondPricingService ==> BondGUIService" << endl;
	std::cout << "BondPricingService ==> BondAlgoStreamingService ==> BondStreamingService ==> bondStreamingHistoricalDataService\n" << endl;
	// build service components
	int throttleVal = 300; // miliseconds
	BondPricingService bondPricingService;
	BondAlgoStreamingService bondAlgoStreamingService;
	BondStreamingService bondStreamingService;
	BondStreamingHistoricalDataConnector bondStreamingHistoricalDataConnector(oStreamPath);
	BondStreamingHistoricalDataService bondStreamingHistoricalDataService(&bondStreamingHistoricalDataConnector);
	BondGUIConnector bondGUIConnector(oGUIPath);
	BondGUIService bondGUIService(throttleVal, &bondGUIConnector);
	
	//build listener
	ToBondAlgoStreamingListener pricingToAlgoStreamingListener(&bondAlgoStreamingService);
	ToBondStreamingListener algoStreamingToStreamingListener(&bondStreamingService);
	ToBondStreamingHistoricalDataListener streamingToStreamingHistoricalDataListener(&bondStreamingHistoricalDataService);
	ToBondGUIListener pricingtoGUIListener(&bondGUIService);

	// link the service components
	bondPricingService.AddListener(&pricingToAlgoStreamingListener);
	bondPricingService.AddListener(&pricingtoGUIListener);
	bondAlgoStreamingService.AddListener(&algoStreamingToStreamingListener);
	bondStreamingService.AddListener(&streamingToStreamingHistoricalDataListener);

	// start
	tm.Start();
	BondPricingConnector bondPricingConnector(iPricePath, &bondPricingService, &bondProductService);
	tm.Stop();
	std::cout << "Time spent: " << tm.GetTime() << " seconds\n"<<endl;
	tm.Reset();

	std::cout << "inquiry.txt ==> allinquiry.txt"<<endl;
	std::cout << "Data flow: " << endl;
	std::cout << "BondInquiryService ==> bondInquiryHistoricalDataService\n" << endl;

	// build service components
	BondInquiryService bondInquiryService;
	BondInquiryHistoricalDataConnector bondInquiryHistoricalDataConnector(oInquiryPath);
	BondInquiryHistoricalDataService bondInquiryHistoricalDataService(&bondInquiryHistoricalDataConnector);

	//build Listener
	ToBondInquiryListener bondInquiryListener(&bondInquiryService);
	ToBondInquiryHistoricalDataListener InquirytoHistoricalDataListener(&bondInquiryHistoricalDataService);

	// link the service components
	bondInquiryService.AddListener(&InquirytoHistoricalDataListener);
	bondInquiryService.AddListener(&bondInquiryListener);

	// start
	tm.Start();
	BondInquiryConnector bondInquiryConnector(iInquiryPath, &bondInquiryService, &bondProductService);
	tm.Stop();
	std::cout << "Time spent: " << tm.GetTime() << " seconds"<<endl;
	tm.Reset();

	std::cout << "==============================================================" << endl;

	system("PAUSE");
	return 0;
}
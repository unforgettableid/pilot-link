#ifndef _PILOT_EXPENSE_H_
#define _PILOT_EXPENSE_H_

#include "pi-args.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Expense_Creator makelong("exps")
#define Expense_DB "ExpenseDB"

enum ExpenseSort {
	esDate, esType
};

enum ExpenseDistance {
	edMiles, edKilometers
};

enum ExpensePayment {
	epAmEx, epCash, epCheck, epCreditCard, epMasterCard, epPrepaid, epVISA, epUnfiled
};

enum ExpenseType {
	etAirfare, etBreakfast, etBus, etBusinessMeals, etCarRental, etDinner,
	etEntertainment, etFax, etGas, etGifts, etHotel, etIncidentals, etLaundry,
	etLimo, etLodging, etLunch, etMileage, etOther, etParking, etPostage,
	etSnack, etSubway, etSupplies, etTaxi, etTelephone, etTips, etTolls,
	etTrain
};

struct ExpenseCustomCurrency {
	char name[16];
	char symbol[4];
	char rate[8];
};

struct Expense {
	struct tm date;
	enum ExpenseType type;
	enum ExpensePayment payment;
	int currency;
	char * amount;
	char * vendor;
	char * city;
	char * attendees;
	char * note;
};

struct ExpenseAppInfo {
  int renamedcategories;
  char CategoryName[16][16];
  unsigned char CategoryID[16];
  unsigned char lastUniqueID;
  enum ExpenseSort sortOrder;
  struct ExpenseCustomCurrency currencies[4];
};

#define Expense_Pref 1

struct ExpensePrefs {
  int currentCategory;
  int defaultCategory;
  int noteFont;
  int showAllCategories;
  int showCurrency;
  int saveBackup;
  int allowQuickFill;
  enum ExpenseDistance unitOfDistance;
  int currencies[7];
};

extern void free_Expense PI_ARGS((struct Expense *));
extern void unpack_Expense PI_ARGS((struct Expense *, unsigned char * record, int len));
extern void pack_Expense PI_ARGS((struct Expense *, unsigned char * record, int * len));
extern void unpack_ExpensePrefs PI_ARGS((struct ExpensePrefs *, unsigned char * record, int len));
extern void pack_ExpensePrefs PI_ARGS((struct ExpensePrefs *, unsigned char * record, int * len));
extern void unpack_ExpenseAppInfo PI_ARGS((struct ExpenseAppInfo *, unsigned char * AppInfo, int len));
extern void pack_ExpenseAppInfo PI_ARGS((struct ExpenseAppInfo *, unsigned char * AppInfo, int * len));

#ifdef __cplusplus
}
#endif

#endif /* _PILOT_EXPENSE_H_ */
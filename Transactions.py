from datetime import datetime, date, timedelta
import logging
from decimal import Decimal, setcontext, BasicContext

from sqlalchemy import Column, Integer, String, ForeignKey, create_engine, Boolean, Numeric, Date
from sqlalchemy.orm import relationship, backref
from Base import Base

# context with ROUND_HALF_UP
setcontext(BasicContext)



class ComparableMixin():
    "Assumes that __lt__ is appropriately implemented and derives the remaining comparison methods from these"
    
    def __ge__(self, other) -> bool:
        return not self.__lt__(other)

    def __gt__(self, other) -> bool:
        return other.__lt__(self)

    def __le__(self, other) -> bool:
        return not self.__gt__(other)

    def __eq__(self, other) -> bool:
        return not self.__lt__(other) and not self.__gt__(other)

    def __ne__(self, other) -> bool:
        return not self.__eq__(other)

class Transaction(ComparableMixin, Base):

    __tablename__ = "transaction"
    _amt = Column(Numeric)
    _date = Column(Date)
    _exempt = Column(Boolean)
    _id = Column(Integer, primary_key = True)
    _account_id = Column(Integer, ForeignKey("account._account_number"))

    def __init__(self, amt, acct_num, date=None, exempt=False):
        """
        Args:
            amt (Decimal): Decimal object representing dollar amount of the transaction.
            acct_num (int): Account number used for logging the transaction's creation.
            date (Date, optional): Date object representing the date the transaction was created.Defaults to None.
            exempt (bool, optional): Determines whether the transaction is exempt from account limits. Defaults to False.
        """       
        self._amt = amt
        self._date = date
        if not self._date:
            self._date = datetime.now().date()
        self._exempt = exempt
        logging.debug(f"Created transaction: {acct_num}, {self._amt}")

    @property
    def date(self):
        # exposes the date as a read-only property to facilitate new
        # functionality in Account
        return self._date

    def __str__(self):
        return f"{self._date}, ${self._amt:,.2f}"

    def is_exempt(self):
        return self._exempt

    def in_same_day(self, other):
        return self._date == other._date

    def in_same_month(self, other):
        return self._date.month == other._date.month and self._date.year == other._date.year

    def __radd__(self, other):
        # allows us to use sum() with transactions
        return other + self._amt

    def check_balance(self, balance):
        return self._amt >= 0 or balance > abs(self._amt)

    def __lt__(self, value):
        return self._date < value._date

    def last_day_of_month(self):
        # Creates a date on the first of the next month (being careful about
        # wrapping around to January) and then subtracts one day
        return date(self._date.year + self._date.month // 12, 
              self._date.month % 12 + 1, 1) - timedelta(1)
        



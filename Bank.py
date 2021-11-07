from Accounts import SavingsAccount, CheckingAccount
from Transactions import Transaction

from sqlalchemy import Column, Integer, String, ForeignKey, create_engine
from sqlalchemy.orm import relationship, backref
from Base import Base

SAVINGS = "savings"
CHECKING = "checking"
class Bank(Base):

    __tablename__ = "bank"
    _accounts = relationship("Account") 
    _id = Column(Integer, primary_key = True)

    def add_account(self, acct_type, amt, session):
        """Creates a new Account object and adds it to this bank object. The Account will be a SavingsAccount or CheckingAccount, depending on the type given.

        Args:
            type (string): "Savings" or "Checking" to indicate the type of account to create
            amt (Decimal): amount for the new transaction representing the initial deposit
        """
        acct_num = self._generate_account_number()
        if acct_type == SAVINGS:
            a = SavingsAccount(acct_num)
        elif acct_type == CHECKING:
            a = CheckingAccount(acct_num)
        else:
            return None
        self._accounts.append(a)
        session.add(a)

        a.add_transaction(amt, session)


    def assess_interest(self):
        for acct in self._accounts:
            acct.assess_interest()

    def assess_fees(self):
        for acct in self._accounts:
            acct.assess_fees()

    def _generate_account_number(self):
        return len(self._accounts) + 1

    def show_accounts(self):
        return self._accounts

    def get_account(self, account_num):
        """Fetches an account by its account number.

        Args:
            account_num (int): account number to seach for

        Returns:
            Account: matching account or None if not found
        """        
        for x in self._accounts:
            if x._account_number == account_num:
                return x
        return None

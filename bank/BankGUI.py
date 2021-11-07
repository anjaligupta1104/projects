import sys
import logging
from decimal import Decimal, InvalidOperation
from datetime import datetime

from sqlalchemy.sql.elements import Over

from Bank import Bank
from Base import Base
from Transactions import Transaction
from Accounts import OverdrawError, TransactionLimitError, TransactionSequenceError
import sqlalchemy
from sqlalchemy.orm.session import sessionmaker
import tkinter as tk
from tkinter import messagebox
from tkcalendar import Calendar, DateEntry

logging.basicConfig(filename='bank.log', level=logging.DEBUG, format='%(asctime)s|%(levelname)s|%(message)s', datefmt='%Y-%m-%d %H:%M:%S')


class BankCLI():
    def __init__(self):
        self._window = tk.Tk()
        self._window.report_callback_exception = handle_exception        
        self._window.title("MY BANK")
        self._options_frame = tk.Frame(self._window)
        self._session = Session()
        self._bank = self._session.query(Bank).first()

        if (self._bank):
            logging.debug("Loaded from bank.db")

        if not self._bank:            
            self._bank = Bank()            
            self._session.add(self._bank)            
            self._session.commit()
            logging.debug("Saved to bank.db")

        self._selected_account = None

        tk.Button(self._options_frame, 
                text="open account", 
                command=self._open_account).grid(row=1, column=1, columnspan=2)
        tk.Button(self._options_frame, 
                text="add transaction", 
                command=self._add_transaction).grid(row=1, column=3)
        tk.Button(self._options_frame, 
                text="interest and fees", 
                command=self._monthly_triggers).grid(row=1, column=4)
        
        # options frame for buttons at top and open account
        # list frame for list of accounts
        # transaction frame for selected account transactions
        # selected frame for add transaction
        self._list_frame = tk.Frame(self._window)
        self._selected_frame = tk.Frame(self._window)
        self._transaction_frame = tk.Frame(self._window)
        self._options_frame.grid(row=0, column=1, columnspan=2)
        self._list_frame.grid(row=1, column=1, columnspan=1, sticky="w")
        self._selected_frame.grid(row=1, column=2, columnspan=1)
        self._transaction_frame.grid(row=1, column=2, columnspan=1)

        self._accounts = {}

        self._window.mainloop()

    def _add_transaction(self):
        def add():
            try:
                self._selected_account.add_transaction(Decimal(e1.get()), self._session, calendar.get_date())
                e1.destroy()
                calendar.destroy()
                b.destroy()
                l1.destroy()
                l2.destroy()
                self._show_accounts()
                self._list_transactions(self._selected_account)
            except InvalidOperation:
                messagebox.showwarning(message="Please try again with a valid dollar amount.")
            except AttributeError:
                messagebox.showwarning(message="This command requires that you first select an account.")
            except OverdrawError:
                messagebox.showwarning(message="This transaction could not be completed due to an insufficient account balance.")
            except TransactionLimitError:
                messagebox.showwarning(message="This transaction could not be completed because the account has reached a transaction limit.")
            except TransactionSequenceError as e:
                messagebox.showwarning(message=f"New transactions must be from {e.latest_date} onward.")

        l1 = tk.Label(self._options_frame, text="Amount:")
        l1.grid(row=2, column=3, sticky="e")
        l2 = tk.Label(self._options_frame, text="Date:")
        l2.grid(row=3, column=3, sticky="e")
        e1 = tk.Entry(self._options_frame, highlightthickness=3)
        e1.grid(row=2, column=4)
        e1.bind("<KeyRelease>", lambda event: self._check_valid_amount(e1, e1.get()))
        calendar = DateEntry(self._options_frame, year=2021, month=10, dawidth=20, borderwidth=2)
        calendar._top_cal.overrideredirect(False)
        calendar.grid(row=3, column=4)
        b = tk.Button(self._options_frame, text="Enter", command=add)
        b.grid(row=4, column=4)

        self._session.commit()
        logging.debug("Saved to bank.db")

    def _show_accounts(self, accounts=None):
        if not accounts:
            accounts = self._bank.show_accounts()
        row = 0
        self.account = tk.IntVar()
        if self._selected_account:
            self.account.set(self._selected_account._account_number)
        for x in accounts:
            self._accounts[x._account_number] = tk.StringVar(value=str(x))
            tk.Radiobutton(self._list_frame, 
                               text=x, 
                               variable=self.account,
                               value=x._account_number,
                               command=lambda acct=x: self._list_transactions(acct)).grid(row=row, column=1)
            row += 1

    def _check_valid_amount(self, e1, amt):
        try:
            Decimal(amt)
            e1.config(highlightcolor="green", highlightbackground="green")
        except InvalidOperation:
            e1.config(highlightcolor="red", highlightbackground="red")
        
    def _open_account(self):
        def add():
            try:
                self._bank.add_account(acct_type.get(), Decimal(e1.get()), self._session)
            except InvalidOperation:
                messagebox.showwarning(message="Please try again with a valid dollar amount.")
            except OverdrawError:
                messagebox.showwarning(message="This transaction could not be completed due to an insufficient account balance.")
            e1.destroy()
            b.destroy()
            l1.destroy()
            w.destroy()
            self._show_accounts()

        l1 = tk.Label(self._options_frame, text="Initial deposit:")
        l1.grid(row=2, column=1)
        e1 = tk.Entry(self._options_frame, highlightthickness=3)
        e1.grid(row=3, column=1)
        e1.bind("<KeyRelease>", lambda event: self._check_valid_amount(e1, e1.get()))

        acct_type = tk.StringVar(self._options_frame)
        w = tk.OptionMenu(self._options_frame, acct_type, "checking", "savings")
        w.grid(row=4, column=1)
        b = tk.Button(self._options_frame, text="Enter", command=add)
        b.grid(row=4, column=2)

        self._session.commit()
        logging.debug("Saved to bank.db")
        
    def _monthly_triggers(self):
        try:
            self._selected_account.assess_interest_and_fees(self._session)
            logging.debug("Triggered fees and interest")
        except AttributeError:
            messagebox.showwarning(message="This command requires that you first select an account.")
        except TransactionSequenceError as e:
            messagebox.showwarning(message=f"Cannot apply interest and fees again in the month of {e.latest_date.strftime('%B')}.")
        self._list_transactions(self._selected_account)
        self._session.commit()
        logging.debug("Saved to bank.db")


    def _list_transactions(self, account):
        self._selected_account = account
        for x in self._transaction_frame.winfo_children():
            x.destroy()
        try:
            row = 0
            for x in self._selected_account.get_transactions():
                if (x._amt > 0):
                    l1 = tk.Label(self._transaction_frame, fg="green", text=x)
                else:
                    l1 = tk.Label(self._transaction_frame, fg="red", text=x)
                l1.grid(row=row, column=2, sticky="nw")
                row += 1
        except AttributeError:
            messagebox.showwarning(message="This command requires that you first select an account.")

def handle_exception(exception, value, traceback):
    messagebox.showwarning(message="Sorry! Something unexpected happened. If this problem persists please contact our support team for assistance.")
    logging.error(f"{exception.__name__}: {repr(value)}")
    sys.exit(1)

if __name__ == "__main__":
    try:
        engine = sqlalchemy.create_engine(f"sqlite:///bank.db")
        Base.metadata.create_all(engine)

        Session = sessionmaker()    
        Session.configure(bind=engine)

        BankCLI()
    except Exception as e:
        print("Sorry! Something unexpected happened. If this problem persists please contact our support team for assistance.")
        logging.error(str(e.__class__.__name__) + ": " + repr(str(e)))

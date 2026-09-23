"use strict";


/* =========================================================
   ELEMENTS
   ========================================================= */


const balance = document.getElementById("balance");
const iban = document.getElementById("iban");

const profileName = document.getElementById("profileName");
const profileAvatar = document.getElementById("profileAvatar");

const transactions = document.getElementById("transactions");

const toast = document.getElementById("toast");
const toastMessage = document.getElementById("toastMessage");

let toastTimer;


/* =========================================================
   DASHBOARD DATA
   ========================================================= */


async function loadDashboard() {

    try {

        const response = await fetch("/Dashboard/Data", {
            method: "GET",
            credentials: "same-origin",
            headers: {
                "Accept": "application/json"
            }
        });


        let data = {};

        try {
            data = await response.json();
        }
        catch {
            data = {};
        }


        /*
         * Authentication/session is invalid.
         */

        if (response.status === 401) {
            window.location.href = "/Login";
            return;
        }


        if (!response.ok || !data.success) {

            showToast(
                data.message || "Unable to load dashboard.",
                "error"
            );

            return;
        }


        updateDashboard(data);

    }
    catch (error) {

        console.error(
            "Dashboard request failed:",
            error
        );

        showToast(
            "Unable to connect to the server.",
            "error"
        );
    }
}


/* =========================================================
   UPDATE UI
   ========================================================= */


function updateDashboard(data) {

    const user = data.user || {};
    const account = data.account || {};


    /*
     * User
     */

    const fullName =
        user.fullName || "User";


    profileName.textContent =
        fullName;

    profileAvatar.textContent =
        getInitial(fullName);


    /*
     * Account balance
     */

    const formattedBalance =
        formatCurrency(account.balance);

    balance.textContent =
        formattedBalance;


    /*
     * IBAN
     */

    if (account.iban) {

        iban.textContent =
            account.iban;

    }
    else {

        iban.textContent =
            "IBAN unavailable";

    }


    /*
     * Transactions
     */

    renderTransactions(
        data.transactions || []
    );
}


/* =========================================================
   CURRENCY
   ========================================================= */


function formatCurrency(value) {

    const amount =
        Number(value);


    if (!Number.isFinite(amount)) {
        return "€ 0.00";
    }


    return new Intl.NumberFormat(
        "en-GB",
        {
            style: "currency",
            currency: "EUR",
            minimumFractionDigits: 2,
            maximumFractionDigits: 2
        }
    ).format(amount);
}


/* =========================================================
   INITIAL
   ========================================================= */


function getInitial(name) {

    if (!name) {
        return "U";
    }


    return name
        .trim()
        .charAt(0)
        .toUpperCase();
}


/* =========================================================
   TRANSACTIONS
   ========================================================= */


function renderTransactions(items) {

    transactions.innerHTML = "";


    if (!items.length) {

        transactions.innerHTML = `
            <div class="transaction-empty">
                No recent transactions.
            </div>
        `;

        return;
    }


    /*
     * Only show the latest five.
     */

    items
        .slice(0, 5)
        .forEach(transaction => {

            const row =
                document.createElement("div");

            row.className =
                "transaction";


            const amount =
                Number(transaction.amount);


            const positive =
                amount >= 0;


            row.innerHTML = `
                <div class="transaction-icon">
                    <span class="icon icon-payment"></span>
                </div>

                <div class="transaction-info">

                    <span class="transaction-name">
                        ${escapeHtml(
                            transaction.description ||
                            "Transaction"
                        )}
                    </span>

                    <span class="transaction-date">
                        ${escapeHtml(
                            transaction.date ||
                            ""
                        )}
                    </span>

                </div>

                <span class="
                    transaction-amount
                    ${positive ? "positive" : "negative"}
                ">
                    ${positive ? "+" : ""}
                    ${formatCurrency(amount)}
                </span>
            `;


            transactions.appendChild(row);
        });
}


/* =========================================================
   HTML ESCAPE
   ========================================================= */


function escapeHtml(value) {

    const element =
        document.createElement("div");

    element.textContent =
        String(value ?? "");


    return element.innerHTML;
}


/* =========================================================
   BUTTONS
   ========================================================= */


document
    .getElementById("transferButton")
    ?.addEventListener("click", () => {

        window.location.href =
            "/Transfer";
    });


document
    .getElementById("paymentButton")
    ?.addEventListener("click", () => {

        window.location.href =
            "/Payments";
    });


document
    .getElementById("qrButton")
    ?.addEventListener("click", () => {

        showToast(
            "QR payments are coming soon.",
            "success"
        );
    });


document
    .getElementById("viewAllButton")
    ?.addEventListener("click", () => {

        window.location.href =
            "/Activity";
    });


/* =========================================================
   PROFILE
   ========================================================= */


document
    .getElementById("profileButton")
    ?.addEventListener("click", () => {

        window.location.href =
            "/Settings";
    });


/* =========================================================
   TOAST
   ========================================================= */


function showToast(message, type = "success") {

    if (!toast || !toastMessage) {
        return;
    }


    toastMessage.textContent =
        message;


    toast.classList.remove(
        "success",
        "error"
    );


    toast.classList.add(
        "active",
        type
    );


    clearTimeout(toastTimer);


    toastTimer = setTimeout(() => {

        toast.classList.remove(
            "active",
            "success",
            "error"
        );

    }, 3500);
}


/* =========================================================
   START
   ========================================================= */


loadDashboard();
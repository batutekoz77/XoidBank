"use strict";


/* =========================================
   ELEMENTS
========================================= */

const twoFAForm =
    document.getElementById("twoFAForm");

const codeInputs = [
    document.getElementById("code1"),
    document.getElementById("code2"),
    document.getElementById("code3"),
    document.getElementById("code4")
];

const codeError =
    document.getElementById("codeError");

const timer =
    document.getElementById("timer");

const verifyButton =
    document.getElementById("verifyButton");

const backToLogin =
    document.getElementById("backToLogin");

const userEmail =
    document.getElementById("userEmail");

const toast =
    document.getElementById("toast");

const toastMessage =
    document.getElementById("toastMessage");


/* =========================================
   STATE
========================================= */

let remainingSeconds = 120;

let timerInterval = null;

let expired = false;


/* =========================================
   EMAIL
========================================= */

/*
 * Backend daha sonra session üzerinden
 * gerçek email'i gönderebilir.
 *
 * Şimdilik backend endpointinden alıyoruz.
 */

async function loadUserEmail() {

    try {

        const response =
            await fetch("/2FA/Email");

        if (!response.ok) {
            return;
        }

        const data =
            await response.json();

        if (
            data.success &&
            data.email
        ) {

            userEmail.textContent =
                data.email;

        }

    }
    catch (error) {

        console.error(
            "Unable to load email:",
            error
        );

    }

}


/* =========================================
   CODE INPUT
========================================= */

codeInputs.forEach(
    (input, index) => {

        input.addEventListener(
            "input",
            () => {

                clearCodeError();


                /*
                 * Sadece rakam kabul et.
                 */

                input.value =
                    input.value.replace(
                        /\D/g,
                        ""
                    );


                /*
                 * Bir rakam yazıldıysa
                 * sonraki kutuya geç.
                 */

                if (
                    input.value &&
                    index < codeInputs.length - 1
                ) {

                    codeInputs[index + 1].focus();

                }


                /*
                 * 4 rakam tamamlandıysa
                 * otomatik submit yapabiliriz.
                 */

                if (
                    codeInputs.every(
                        item => item.value.length === 1
                    )
                ) {

                    twoFAForm.requestSubmit();

                }

            }
        );


        /*
         * Backspace ile önceki kutuya geç.
         */

        input.addEventListener(
            "keydown",
            (event) => {

                if (
                    event.key === "Backspace" &&
                    !input.value &&
                    index > 0
                ) {

                    codeInputs[index - 1].focus();

                }

            }
        );


        /*
         * Paste desteği.
         */

        input.addEventListener(
            "paste",
            (event) => {

                event.preventDefault();

                const pasted =
                    event.clipboardData
                        .getData("text")
                        .replace(/\D/g, "")
                        .slice(0, 4);


                pasted
                    .split("")
                    .forEach(
                        (digit, digitIndex) => {

                            if (
                                codeInputs[digitIndex]
                            ) {

                                codeInputs[
                                    digitIndex
                                ].value = digit;

                            }

                        }
                    );


                if (pasted.length === 4) {

                    codeInputs[3].focus();

                    clearCodeError();

                }

            }
        );

    }
);


/* =========================================
   CODE
========================================= */

function getCode() {

    return codeInputs
        .map(input => input.value)
        .join("");

}


/* =========================================
   ERROR
========================================= */

function clearCodeError() {

    codeError.textContent = "";

    codeInputs.forEach(
        input => {

            input.classList.remove(
                "error"
            );

        }
    );

}


function showCodeError(message) {

    codeError.textContent =
        message;

    codeInputs.forEach(
        input => {

            input.classList.add(
                "error"
            );

        }
    );

}


/* =========================================
   TIMER
========================================= */

function updateTimer() {

    const minutes =
        Math.floor(
            remainingSeconds / 60
        );

    const seconds =
        remainingSeconds % 60;


    timer.textContent =
        String(minutes).padStart(2, "0") +
        ":" +
        String(seconds).padStart(2, "0");

}


function startTimer() {

    updateTimer();


    timerInterval =
        setInterval(
            () => {

                remainingSeconds--;


                if (
                    remainingSeconds <= 0
                ) {

                    remainingSeconds = 0;

                    updateTimer();

                    expire2FA();

                    return;

                }


                updateTimer();

            },
            1000
        );

}


function expire2FA() {

    if (expired) {
        return;
    }


    expired = true;


    clearInterval(
        timerInterval
    );


    timer.classList.add(
        "expired"
    );


    verifyButton.disabled =
        true;


    showToast(
        "Verification code expired.",
        "error"
    );


    /*
     * 120 saniye doldu.
     * Login sayfasına dön.
     */

    setTimeout(
        () => {

            window.location.href =
                "/Login";

        },
        1200
    );

}


/* =========================================
   VERIFY
========================================= */

if (twoFAForm) {

    twoFAForm.addEventListener(
        "submit",
        async (event) => {

            event.preventDefault();


            if (expired) {
                return;
            }


            clearCodeError();


            const code =
                getCode();


            /* -----------------------------
               FRONTEND VALIDATION
            ----------------------------- */

            if (!/^\d{4}$/.test(code)) {

                showCodeError(
                    "Enter the 4-digit verification code."
                );

                return;

            }


            /* -----------------------------
               LOADING
            ----------------------------- */

            verifyButton.classList.add(
                "loading"
            );

            verifyButton.disabled =
                true;


            try {

                /* -------------------------
                   BACKEND REQUEST
                ------------------------- */

                const response =
                    await fetch(
                        "/2FA/Verify",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body:
                                JSON.stringify({
                                    code: code
                                })
                        }
                    );


                /* -------------------------
                   RESPONSE
                ------------------------- */

                let data = {};


                try {

                    data =
                        await response.json();

                }
                catch {

                    data = {};

                }


                /* -------------------------
                   SUCCESS
                ------------------------- */

                if (
                    response.ok &&
                    data.success
                ) {

                    clearInterval(
                        timerInterval
                    );


                    showToast(
                        data.message ||
                        "Verification successful."
                    );


                    setTimeout(
                        () => {

                            window.location.href =
                                "/Dashboard";

                        },
                        500
                    );


                    return;

                }


                /* -------------------------
                   ERROR
                ------------------------- */

                showCodeError(
                    data.message ||
                    "Invalid verification code."
                );

            }
            catch (error) {

                console.error(
                    "2FA request failed:",
                    error
                );


                showToast(
                    "Unable to connect to the server.",
                    "error"
                );

            }


            verifyButton.classList.remove(
                "loading"
            );

            verifyButton.disabled =
                false;

        }
    );

}


/* =========================================
   BACK TO LOGIN
========================================= */

if (backToLogin) {

    backToLogin.addEventListener(
        "click",
        () => {

            clearInterval(
                timerInterval
            );


            window.location.href =
                "/Login";

        }
    );

}


/* =========================================
   TOAST
========================================= */

function showToast(
    message,
    type = "success"
) {

    if (
        !toast ||
        !toastMessage
    ) {
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


    setTimeout(
        () => {

            toast.classList.remove(
                "active",
                "success",
                "error"
            );

        },
        3500
    );

}


/* =========================================
   INITIALIZE
========================================= */

loadUserEmail();

startTimer();

codeInputs[0].focus();
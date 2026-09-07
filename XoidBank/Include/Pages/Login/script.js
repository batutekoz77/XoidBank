// script.js

"use strict";


/* =========================================
   ELEMENTS
========================================= */

const loginForm =
    document.getElementById("loginForm");

const email =
    document.getElementById("email");

const password =
    document.getElementById("password");

const emailError =
    document.getElementById("emailError");

const passwordError =
    document.getElementById("passwordError");

const passwordToggle =
    document.getElementById("passwordToggle");

const loginButton =
    document.getElementById("loginButton");

const forgotButton =
    document.getElementById("forgotButton");

const createAccountButton =
    document.getElementById("createAccountButton");

const rememberMe =
    document.getElementById("rememberMe");

const toast =
    document.getElementById("toast");

const toastMessage =
    document.getElementById("toastMessage");


let toastTimer;


/* =========================================
   PASSWORD VISIBILITY
========================================= */

if (passwordToggle) {

    passwordToggle.addEventListener("click", () => {

        const passwordVisible =
            password.type === "text";


        password.type =
            passwordVisible
                ? "password"
                : "text";


        passwordToggle.classList.toggle(
            "visible",
            !passwordVisible
        );


        passwordToggle.setAttribute(
            "aria-label",
            passwordVisible
                ? "Show password"
                : "Hide password"
        );

    });

}


/* =========================================
   CLEAR ERRORS
========================================= */

function clearErrors() {

    emailError.textContent = "";
    passwordError.textContent = "";


    email
        .closest(".input-wrapper")
        .classList.remove("error");


    password
        .closest(".input-wrapper")
        .classList.remove("error");
}


/* =========================================
   VALIDATION
========================================= */

function validateForm() {

    clearErrors();

    let valid = true;


    const emailValue =
        email.value.trim();

    const passwordValue =
        password.value.trim();


    /* -------------------------------------
       EMAIL
    ------------------------------------- */

    if (!emailValue) {

        emailError.textContent =
            "Enter your email address.";


        email
            .closest(".input-wrapper")
            .classList.add("error");


        valid = false;

    }
    else {

        const emailPattern =
            /^[^\s@]+@[^\s@]+\.[^\s@]+$/;


        if (
            emailValue.length > 254 ||
            !emailPattern.test(emailValue)
        ) {

            emailError.textContent =
                "Enter a valid email address.";

            email
                .closest(".input-wrapper")
                .classList.add("error");

            valid = false;
        }

    }


    /* -------------------------------------
       PASSWORD
    ------------------------------------- */

    if (!passwordValue) {

        passwordError.textContent =
            "Enter your password.";


        password
            .closest(".input-wrapper")
            .classList.add("error");


        valid = false;

    }
    else if (passwordValue.length < 6) {

        passwordError.textContent =
            "Your password must contain at least 6 characters.";

        password
            .closest(".input-wrapper")
            .classList.add("error");

        valid = false;

    }
    else if (passwordValue.length > 128) {

        passwordError.textContent =
            "Your password must not exceed 128 characters.";

        password
            .closest(".input-wrapper")
            .classList.add("error");

        valid = false;
    }


    return valid;
}


/* =========================================
   LIVE ERROR CLEARING
========================================= */

if (email) {

    email.addEventListener("input", () => {

        emailError.textContent = "";

        email
            .closest(".input-wrapper")
            .classList.remove("error");

    });

}


if (password) {

    password.addEventListener("input", () => {

        passwordError.textContent = "";

        password
            .closest(".input-wrapper")
            .classList.remove("error");

    });

}


/* =========================================
   LOGIN
========================================= */

if (loginForm) {

    loginForm.addEventListener(
        "submit",
        async (event) => {

            event.preventDefault();


            /* -----------------------------
               VALIDATION
            ----------------------------- */

            if (!validateForm()) {
                return;
            }


            /* -----------------------------
               VALUES
            ----------------------------- */

            const emailValue =
                email.value.trim();

            const passwordValue =
                password.value;

            const rememberMeValue =
                rememberMe
                    ? rememberMe.checked
                    : false;


            /* -----------------------------
               BUTTON LOADING
            ----------------------------- */

            loginButton.classList.add(
                "loading"
            );

            loginButton.disabled = true;


            try {

                /* -------------------------
                   LOGIN REQUEST
                ------------------------- */

                const response =
                    await fetch(
                        "/Login/Submit",
                        {
                            method: "POST",

                            headers: {
                                "Content-Type":
                                    "application/json"
                            },

                            body: JSON.stringify({

                                email:
                                    emailValue,

                                password:
                                    passwordValue,

                                rememberMe:
                                    rememberMeValue

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

                    showToast(
                        data.message ||
                        "Login successful.",
                        "success"
                    );


                    /*
                     * Login başarılı.
                     * Dashboard HTML sayfasına git.
                     */

                    setTimeout(() => {

                        window.location.href =
                            "/2FA";

                    }, 500);


                    return;
                }


                /* -------------------------
                   LOGIN ERROR
                ------------------------- */

                showToast(
                    data.message ||
                    "Login failed.",
                    "error"
                );

            }
            catch (error) {

                console.error(
                    "Login request failed:",
                    error
                );


                showToast(
                    "Unable to connect to the server.",
                    "error"
                );

            }


            /* -----------------------------
               RESET BUTTON
            ----------------------------- */

            loginButton.classList.remove(
                "loading"
            );

            loginButton.disabled = false;

        }
    );

}


/* =========================================
   FORGOT PASSWORD
========================================= */

if (forgotButton) {

    forgotButton.addEventListener(
        "click",
        () => {

            window.location.href =
                "/ForgotCredentials";

        }
    );

}


/* =========================================
   CREATE ACCOUNT / REGISTER
========================================= */

if (createAccountButton) {

    createAccountButton.addEventListener(
        "click",
        () => {

            window.location.href =
                "/Register";

        }
    );

}


/* =========================================
   TOAST
========================================= */

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

    clearTimeout(
        toastTimer
    );

    toastTimer =
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
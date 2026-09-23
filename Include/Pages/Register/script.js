"use strict";


/* =========================================
   ELEMENTS
========================================= */

const registerForm =
    document.getElementById("registerForm");

const fullName =
    document.getElementById("fullName");

const email =
    document.getElementById("email");

const phoneNumber =
    document.getElementById("phoneNumber");

const BSN =
    document.getElementById("BSN");

const password =
    document.getElementById("password");

const confirmPassword =
    document.getElementById("confirmPassword");

const terms =
    document.getElementById("terms");

const passwordToggle =
    document.getElementById("passwordToggle");

const registerButton =
    document.getElementById("registerButton");

const loginButton =
    document.getElementById("loginButton");


const fullNameError =
    document.getElementById("fullNameError");

const emailError =
    document.getElementById("emailError");

const phoneNumberError =
    document.getElementById("phoneNumberError");

const BSNError =
    document.getElementById("BSNError");

const passwordError =
    document.getElementById("passwordError");

const confirmPasswordError =
    document.getElementById("confirmPasswordError");

const termsError =
    document.getElementById("termsError");


const toast =
    document.getElementById("toast");

const toastMessage =
    document.getElementById("toastMessage");


let toastTimer;


/* =========================================
   PASSWORD VISIBILITY
========================================= */

if (passwordToggle) {

    passwordToggle.addEventListener(
        "click",
        () => {

            const visible =
                password.type === "text";


            password.type =
                visible
                    ? "password"
                    : "text";


            passwordToggle.classList.toggle(
                "visible",
                !visible
            );


            passwordToggle.setAttribute(
                "aria-label",
                visible
                    ? "Show password"
                    : "Hide password"
            );

        }
    );

}


/* =========================================
   ERROR HELPERS
========================================= */

function clearErrors() {

    fullNameError.textContent = "";
    emailError.textContent = "";
    phoneNumberError.textContent = "";
    BSNError.textContent = "";
    passwordError.textContent = "";
    confirmPasswordError.textContent = "";
    termsError.textContent = "";


    document
        .querySelectorAll(".input-wrapper")
        .forEach(wrapper => {

            wrapper.classList.remove("error");

        });

}


function setInputError(
    input,
    errorElement,
    message
) {

    errorElement.textContent =
        message;


    input
        .closest(".input-wrapper")
        .classList.add("error");

}


/* =========================================
   VALIDATION
========================================= */

function validateForm() {

    clearErrors();

    let valid = true;


    const fullNameValue =
        fullName.value.trim();

    const emailValue =
        email.value.trim();

    const phoneValue =
        phoneNumber.value.trim();

    const BSNValue =
        BSN.value.trim();

    const passwordValue =
        password.value.trim();

    const confirmPasswordValue =
        confirmPassword.value.trim();


    /* -------------------------------------
       FULL NAME
    ------------------------------------- */

    if (!fullNameValue) {

        setInputError(
            fullName,
            fullNameError,
            "Enter your full name."
        );

        valid = false;

    }
    else if (fullNameValue.length > 100) {

        setInputError(
            fullName,
            fullNameError,
            "Your name must not exceed 100 characters."
        );

        valid = false;

    }


    /* -------------------------------------
       EMAIL
    ------------------------------------- */

    const emailPattern =
        /^[^\s@]+@[^\s@]+\.[^\s@]+$/;


    if (!emailValue) {

        setInputError(
            email,
            emailError,
            "Enter your email address."
        );

        valid = false;

    }
    else if (
        emailValue.length > 254 ||
        !emailPattern.test(emailValue)
    ) {

        setInputError(
            email,
            emailError,
            "Enter a valid email address."
        );

        valid = false;

    }


    /* -------------------------------------
       PHONE
    ------------------------------------- */

    if (phoneValue.length > 20) {

        setInputError(
            phoneNumber,
            phoneNumberError,
            "Phone number must not exceed 20 characters."
        );

        valid = false;

    }


    /* -------------------------------------
       BSN
    ------------------------------------- */

    if (BSNValue.length > 9) {

        setInputError(
            BSN,
            BSNError,
            "BSN must not exceed 9 characters."
        );

        valid = false;

    }


    /* -------------------------------------
       PASSWORD
    ------------------------------------- */

    if (!passwordValue) {

        setInputError(
            password,
            passwordError,
            "Enter your password."
        );

        valid = false;

    }
    else if (passwordValue.length < 6) {

        setInputError(
            password,
            passwordError,
            "Your password must contain at least 6 characters."
        );

        valid = false;

    }
    else if (passwordValue.length > 128) {

        setInputError(
            password,
            passwordError,
            "Your password must not exceed 128 characters."
        );

        valid = false;

    }


    /* -------------------------------------
       CONFIRM PASSWORD
    ------------------------------------- */

    if (!confirmPasswordValue) {

        setInputError(
            confirmPassword,
            confirmPasswordError,
            "Confirm your password."
        );

        valid = false;

    }
    else if (
        confirmPasswordValue !== passwordValue
    ) {

        setInputError(
            confirmPassword,
            confirmPasswordError,
            "Passwords do not match."
        );

        valid = false;

    }


    /* -------------------------------------
       TERMS
    ------------------------------------- */

    if (!terms.checked) {

        termsError.textContent =
            "You must accept the terms and privacy policy.";

        valid = false;

    }


    return valid;
}


/* =========================================
   LIVE ERROR CLEARING
========================================= */

[
    fullName,
    email,
    phoneNumber,
    BSN,
    password,
    confirmPassword
].forEach(input => {

    input.addEventListener(
        "input",
        () => {

            input
                .closest(".input-wrapper")
                .classList.remove("error");

            const error =
                input
                    .closest(".form-group")
                    .querySelector(".error-message");

            if (error) {
                error.textContent = "";
            }

        }
    );

});


terms.addEventListener(
    "change",
    () => {

        termsError.textContent = "";

    }
);


/* =========================================
   REGISTER
========================================= */

registerForm.addEventListener(
    "submit",
    async (event) => {

        event.preventDefault();


        if (!validateForm()) {
            return;
        }


        const data = {

            fullName:
                fullName.value.trim(),

            email:
                email.value.trim(),

            password:
                password.value.trim(),

            confirmPassword:
                confirmPassword.value.trim(),

            phoneNumber:
                phoneNumber.value.trim(),

            BSN:
                BSN.value.trim()

        };


        registerButton.classList.add(
            "loading"
        );

        registerButton.disabled = true;


        try {

            const response =
                await fetch(
                    "/Register/Submit",
                    {
                        method: "POST",
                        headers: {
                            "Content-Type": "application/json"
                        },
                        body: JSON.stringify(data)
                    }
                );


            let result = {};

            try {

                result =
                    await response.json();

            }
            catch {

                result = {};

            }


            if (
                response.ok &&
                result.success
            ) {

                showToast(
                    result.message ||
                    "Account created successfully.",
                    "success"
                );


                setTimeout(() => {

                    window.location.href =
                        "/Login";

                }, 700);


                return;
            }


            showToast(
                result.message ||
                "Unable to create your account.",
                "error"
            );

        }
        catch (error) {

            console.error(
                "Register request failed:",
                error
            );


            showToast(
                "Unable to connect to the server.",
                "error"
            );

        }


        registerButton.classList.remove(
            "loading"
        );

        registerButton.disabled = false;

    }
);


/* =========================================
   LOGIN BUTTON
========================================= */

loginButton.addEventListener(
    "click",
    () => {

        window.location.href =
            "/Login";

    }
);


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
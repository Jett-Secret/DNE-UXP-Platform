/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * Details of supported OAuth2 Providers.
 */
var EXPORTED_SYMBOLS = ["OAuth2Providers"];

var {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "gPrefService", function() {
  return Services.prefs;
});

// map of hostnames to [issuer, scope]
var kHostnames = new Map([
  ["imap.googlemail.com", ["accounts.google.com", "https://mail.google.com/"]],
  ["smtp.googlemail.com", ["accounts.google.com", "https://mail.google.com/"]],
  ["imap.gmail.com", ["accounts.google.com", "https://mail.google.com/"]],
  ["smtp.gmail.com", ["accounts.google.com", "https://mail.google.com/"]],

  ["imap.mail.ru", ["o2.mail.ru", "mail.imap"]],
  ["smtp.mail.ru", ["o2.mail.ru", "mail.imap"]],
]);

// map of issuers to appKey, appSecret, authURI, tokenURI

// For the moment, these details are user-provided, since there is no way for
// unverified applications to get a proper OAuth2 key.
var kIssuers = new Map ([
  ["accounts.google.com", [
    gPrefService.getCharPref("oauth2.google.clientid"),
    gPrefService.getCharPref("oauth2.google.clientsecret"),
    'https://accounts.google.com/o/oauth2/auth',
    'https://oauth2.googleapis.com/token'
  ]],
  ["o2.mail.ru", [
    gPrefService.getCharPref("oauth2.mailru.clientid"),
    gPrefService.getCharPref("oauth2.mailru.clientsecret"),
    'https://o2.mail.ru/login',
    'https://o2.mail.ru/token'
  ]],
]);

/**
 *  OAuth2Providers: Methods to lookup OAuth2 parameters for supported
 *                   email providers.
 */
var OAuth2Providers = {

  /**
   * Map a hostname to the relevant issuer and scope.
   *
   * @param aHostname  String representing the url for an imap or smtp
   *                   server (example "imap.googlemail.com").
   *
   * @returns          Array with [issuer, scope] for the hostname if found,
   *                   else undefined. issuer is a string representing the
   *                   organization, scope is an oauth parameter describing\
   *                   the required access level.
   */
  getHostnameDetails: function (aHostname) { return kHostnames.get(aHostname);},

  /**
   * Map an issuer to OAuth2 account details.
   *
   * @param aIssuer    The organization issuing oauth2 parameters, example
   *                   "accounts.google.com".
   *
   * @return           Array containing [appKey, appSecret, authURI, tokenURI]
   *                   where appKey and appDetails are strings representing the
   *                   account registered for Thunderbird with the organization,
   *                   authURI and tokenURI are url strings representing
   *                   endpoints to access OAuth2 authentication.
   */
  getIssuerDetails: function (aIssuer) { return kIssuers.get(aIssuer);}
}

/*
 * You may redistribute this program and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CRYPTO_AUTH_H
#define CRYPTO_AUTH_H

#include "benc/Object.h"
#include "interface/Interface.h"
#include "memory/Allocator.h"
#include "util/Endian.h"
#include "util/Log.h"

#include <stdint.h>
#include <stdbool.h>
#include <event2/event.h>

struct CryptoAuth;

/**
 * Associate a password:authtype pair with a user object.
 * Calling CryptoAuth_getUser() on any interface which has established a connection with
 * the same password:authType pair will return the same user object.
 *
 * @param password This should be a key derived from the password using a good key derivation
 *                 function, using a plaintext password here is NOT recommended.
 * @param authType The method of authenticating the user, only option currently is 1 for sha256
 *                 based authentication.
 * @param user The thing to associate with this user, will be returned by CryptoAuth_getUser().
 *             If this is NULL and requireAuthentication is enabled, authentication will fail.
 * @param context The CryptoAuth context.
 * @return 0 if all goes well, -1 if the authentication method is not supported and -2 if there is
 *         not enough space to store the user.
 */
int32_t CryptoAuth_addUser(String* password,
                           uint8_t authType,
                           void* user,
                           struct CryptoAuth* context);

/**
 * Get the user object associated with the authenticated session or NULL if there is none.
 * Please make sure to only call this on interfaces which were actually returned by
 * CryptoAuth_wrapInterface() as strange and interesting bugs will result otherwise.
 *
 * @param interface an interface as returned by CryptoAuth_wrapInterface().
 * @return the user object added by calling CryptoAuth_addUser() or NULL if this session is not
 *         authenticated.
 */
void* CryptoAuth_getUser(struct Interface* interface);

/**
 * Create a new crypto authenticator.
 *
 * @param config the configuration for this CryptoAuth, configuration options include:
 *            resetAfterInactivitySeconds -- the number of seconds of inactivity after which to
 *                                           reset the connection.
 * @param allocator the means of aquiring memory.
 * @param privateKey the private key to use for this CryptoAuth or null if one should be generated.
 * @param eventBase the libevent context for handling timeouts.
 * @param logger the mechanism for logging output from the CryptoAuth.
 *               if NULL then no logging will be done.
 * @return a new CryptoAuth context.
 */
struct CryptoAuth* CryptoAuth_new(Dict* config,
                                  struct Allocator* allocator,
                                  const uint8_t* privateKey,
                                  struct event_base* eventBase,
                                  struct Log* logger);

/**
 * Wrap an interface with crypto authentication.
 *
 * @param toWarp the interface to wrap
 * @param herPublicKey the public key of the other party or NULL if unknown.
 * @param requireAuth if the remote end of this interface begins the connection, require
 *                    them to present valid authentication credentials to connect.
 *                    If this end begins the connection, this parameter has no effect.
 * @param authenticatePackets if true, all packets will be protected against forgery and replay
 *                            attacks, this is a seperate system from password and authType.
 * @param context the CryptoAuth context.
 */
struct Interface* CryptoAuth_wrapInterface(struct Interface* toWrap,
                                           uint8_t herPublicKey[32],
                                           const bool requireAuth,
                                           bool authenticatePackets,
                                           struct CryptoAuth* context);

/**
 * Choose the authentication credentials to use.
 * WARNING: Even if the remote end begins the connection, these credentials will be presented which
 *          will cause the connection initiation to fail if the remote end does not know of them.
 *
 * @param password the password to use for authenticating, this must match the password given to
 *                 CryptoAuth_addUser() at the other end of the connection.
 * @param authType this must match CryptoAuth_addUser() at the other end of the connection.
 * @param wrappedInterface this MUST be the output from CryptoAuth_wrapInterface().
 */
void CryptoAuth_setAuth(const String* password,
                        const uint8_t authType,
                        struct Interface* wrappedInterface);

void CryptoAuth_getPublicKey(uint8_t output[32], struct CryptoAuth* context);

uint8_t* CryptoAuth_getHerPublicKey(struct Interface* interface);

void CryptoAuth_reset(struct Interface* interface);

#endif

# Networked-Game






HOW IT WORKS:
    In this we use SELECT() in the berkly socket programming C++ to handle multiple users.

    We use a variety of classes to handle multiple clients and extensive functionality.

    An example is the 'Test Timer' which is triggered when the user sends '!time'. The server will create an object
    with a time-stamp and that client. This allows the server to still handle and respond to other users.
    When the same client send a '!time', the timer ends and the array of objects is marked with a sock value of 0.

    /***************************************************************************************************************/

    This is then further implemented into lobbies. Where each time the user submits one of the words, it is kept in and
    the following word is given out.

    Lobbies will also be a class.
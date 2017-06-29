<?php

/**
 *
 */

class IDER_Callback
{

    static function handler($userInfo)
    {

        $userInfo = IDER_UserInfoManager::normalize($userInfo);

        // TODO callback function
        $handled = module_invoke_all('before_callback_handler', $userInfo, $_SESSION['openid_connect_scope']);

        // if user function hadn't been exclusive let's resume the standard flow
        if (!$handled) {
            self::defaultHandler($userInfo);
        }

    }


    // register or authenticate user
    static function defaultHandler($userInfo)
    {
        // check if user exists by email
        // ps: if user uses same email on a new IDer profile the sub will be updated on the old profile

        $user = user_load_by_mail($userInfo->email);


        // check if user exists by sub
        if (!$user) {

            $user = user_load_multiple(
                array(),
                array(
                    'ider_sub' => $userInfo->sub
                )
            );
            // TODO: leverage future endpoint to check which side changed the email: local->no access and error msg, remote->update email

        }

        // if new, register first
        if (!$user) {

            $user = self::_do_register($userInfo);

        }

        // Log the User In
        self::_login($user->uid);

        // update user data
        self::_update_ider_sub((int)$user->uid, $userInfo->sub);

        if (user_is_logged_in()) {
            // pass the controll to user defined functions and landing pages
            //module_invoke_all('after_callback_handler', $userInfo, $_SESSION['openid_connect_scope']);

            drupal_goto(url(variable_get('ider_login_welcome_page')));

            exit;

        }

        self::access_denied("User unable to login.");
    }

    /**
     * Show error message if the user doesn't have access.
     */
    static function access_denied($errormsg)
    {
        if (is_null($errormsg)) {
            $errormsg = "Error authenticating user";
        }

        drupal_set_message(t($errormsg), 'error');

        drupal_goto();

    }

    /**
     * Update IDer database sub
     */
    private static function _update_ider_sub($userId, $userSub)
    {

        // load the user by its ID
        $account = user_load($userId);

        // build the key value edit array
        $edit = array('ider_sub' => $userSub);

        // save the user to the database edited
        user_save($account, $edit);

        // TMP: filling missing fields

    }

    /**
     * Register an user
     */
    private static function _do_register($userInfo)
    {

        // Does not have an account. Register and then log the user in
        $random_password =  user_password(12);

        // build the new user Array()
        $newUser = array(
            'ider_sub' => $userInfo->sub,
            'name' => $userInfo->mail,
            'mail' => $userInfo->mail,
            'pass' => $random_password,
            'status' => 1,
            'access' => REQUEST_TIME,
            'roles' => array(), // No other roles than Authenticated
        );

        // save the new user
        $user = user_save(NULL, $newUser);

        // destroy eventual login sessions
        module_invoke_all('user_logout', $user);

        return $user;

    }

    /**
     * Login an user by its ID
     */
    private static function _login($userID)
    {

        // User ID 1 is not allowed
        if ('1' === $userID) {

            // set the error message
            self::access_denied("For security reasons, admin cannot login via IDer.");
            // logout an user redirecting it
            user_logout();

        } else {

            // check if user is not logged in
            if (!user_is_logged_in()) {

                // make the key value pairs array with the info of the user to log in
                $formState = array('uid' => (int)$userID);
                // submit the data
                user_login_submit(array(), $formState);

            }

            return true;

        }

    }


}
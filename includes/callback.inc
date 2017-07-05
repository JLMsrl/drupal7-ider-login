<?php

/**
 *
 */

class IDER_Callback
{

    static function handler($userInfo)
    {

        $userInfo = IDER_UserInfoManager::normalize($userInfo);

        $handled = reset(module_invoke_all('before_callback_handler', $userInfo, $_SESSION['openid_connect_scope']));

        // if user function hadn't been exclusive let's resume the standard flow
        if (!$handled) {
            self::defaultHandler($userInfo);
        }

    }


    // register or authenticate user
    static function defaultHandler($userInfo)
    {
        // ps: if user uses same email on a new IDer profile the sub will be updated on the old profile

        // check if user exists by email
        $user = user_load_by_mail($userInfo->email);

        // check if user exists by sub
        if (!$user) {

            $user = user_load_multiple(
                array(),
                array(
                    'ider_sub' => $userInfo->sub
                )
            );

            $user = reset($user);

        }

        // if new, register first
        if (!$user) {

            $user = self::_do_register($userInfo);

        }

        // update user sub
        self::_update_ider_sub($user->uid, $userInfo->sub);

        // unset the sub
        unset($userInfo->sub);

        // check for email changes
        if($user->mail !== $userInfo->email){

            if(self::_local_mail_identical($user->uid, $user->mail)){

                self::_update_user_mail($user->uid, $userInfo->email);

            }else{

                self::user_logout();

                self::access_denied('Update the IDer email first!');

                drupal_goto();

            }

        }

        // Log the User In
        self::_login($user->uid);

        // update the user info into IDer table
        self::_update_ider_table($user->uid, $userInfo);

        if (user_is_logged_in()) {

            // pass the controll to user defined functions and landing pages
            module_invoke_all('after_callback_handler', $userInfo, $_SESSION['openid_connect_scope']);

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

    }

    /**
     * Logout the user
     */
    static function user_logout()
    {
        global $user;

        module_invoke_all('user_logout', $user);

        @session_destroy();

    }

    /**
     * Update IDer database sub
     */
    private static function _update_ider_sub($userID, $userSub)
    {
        // load the accountc
        $account = user_load($userID);

        // build the key value edit array
        $edit = array('ider_sub' => $userSub);

        // save the user to the database edited
        user_save($account, $edit);

    }

    /**
     * Add a record foreach field inside IDer table
     */
    private static function _update_ider_table($userID, $userInfo)
    {

        foreach ($userInfo as $key => $value) {

            // write the record uncoditionally
            db_merge('ider_user_data')
                ->key(
                    array(
                        'uid' => $userID,
                        'user_field' => $key,
                    )
                )
                ->fields(array(
                    'uid' => $userID,
                    'user_field' => $key,
                    'user_value' => $value
                ))
                ->execute();

        }

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
            'name' => $userInfo->email,
            'mail' => $userInfo->email,
            'pass' => $random_password,
            'status' => 1,
            'access' => REQUEST_TIME,
            'roles' => array(), // No other roles than Authenticated
        );

        // save the new user
        $user = user_save(NULL, $newUser);

        // destroy eventual login sessions
        self::user_logout();

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
                $formState = array('uid' => $userID);

                // submit the data
                user_login_submit(array(), $formState);

            }

            return true;

        }

    }

    /**
     * Check if the local mail are identical
     */
    private static function _local_mail_identical($userID, $userMail)
    {
        $areIdentical = true;

        $iderLocalEmail = db_select('ider_user_data', 'i')
            ->fields('i')
            ->condition('id', $userID, '=')
            ->condition('user_field', 'email', '=')
            ->condition('user_value', $userMail, '=')
            ->execute()
            ->fetchAssoc();

        if(!$iderLocalEmail){

            $areIdentical = false;

        }

        return $areIdentical;

    }

    /**
     * Update the old mail with a new one
     */
    private static function _update_user_mail($userID, $mail)
    {

        // load the accountc
        $account = user_load($userID);

        // build the key value edit array
        $edit = array('mail' => $mail);

        // save the user to the database edited
        user_save($account, $edit);

    }


}
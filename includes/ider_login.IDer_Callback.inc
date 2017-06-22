<?php

/**
 *
 */

class IDER_Callback
{

    static function handler($user_info)
    {
        $user_info = IDER_UserInfoManager::normalize($user_info);
        // TODO callback function
        IDER_Callback::defaultHandler($user_info);

    }


    // register or authenticate user
    static function defaultHandler($user_info)
    {
        // check if user exists by email
        // ps: if user uses same email on a new IDer profile the sub will be updated on the old profile
        //$user = get_user_by('email', $user_info->email);

        $user = user_load_by_mail($user_info->email);


        // check if user exists by sub
        if (!$user) {
            $user = user_load_multiple(
                array(),
                array(
                    'ider_sub' => $user_info->sub
                )
            );
            // TODO: leverage future endpoint to check which side changed the email: local->no access and error msg, remote->update email
        }

        // if new, register first
        if (!$user) {

            $user = self::_do_register($user_info);

        }

        // Log the User In
        self::_login($user->uid);

        // update user data
        self::_update_ider_sub((int)$user->uid, $user_info->sub);

        if (user_is_logged_in()) {
            // pass the controll to user defined functions and landing pages
            //apply_filters('after_callback_handler', $user_info, $_SESSION['openid_connect_scope']);

            drupal_goto(url(variable_get('ider_login_welcome_page')));

        }

        IDER_Callback::access_denied("User unable to login.");
    }


    static function access_denied($errormsg)
    {
        if (is_null($errormsg)) {
            $errormsg = "Error authenticating user";
        }

        drupal_set_message(t($errormsg), 'error');
        drupal_goto(url('/'));

    }


    private static function _update_ider_sub($user_id, $userSub)
    {
        $account = user_load($user_id);
        $edit = array('ider_sub' => $userSub);
        user_save($account, $edit);

        // TMP: filling missing fields
        /*
        update_user_meta($user_id, 'billing_address_1', 'Via Roma 10');
        update_user_meta($user_id, 'billing_postcode', '10100');
        update_user_meta($user_id, 'billing_city', 'Torino');
        update_user_meta($user_id, 'billing_state', 'TO');
        */

    }


    private static function _do_register($user_info)
    {

        // Does not have an account. Register and then log the user in
        $random_password =  user_password(12);

        $new_user = array(
            'ider_sub' => $user_info->sub,
            'name' => $user_info->mail,
            'mail' => $user_info->mail,
            'pass' => $random_password,
            'status' => 1,
            'access' => REQUEST_TIME,
            'roles' => array(), // No other roles than Authenticated
        );

        $user = user_save(NULL, $new_user);

        drupal_session_destroy_uid((int)$user->uid);

        return $user;
    }


    private static function _login($user_to_login)
    {

        // User ID 1 is not allowed
        if ('1' === (int)$user_to_login) {

            drupal_session_destroy_uid((int)$user_to_login);
            self::access_denied("For security reasons, admin cannot login via IDer.");
            return false;

        } else {

            if (!user_is_logged_in()) {

                $formState = array('uid' => (int)$user_to_login);
                user_login_submit(array(), $formState);

            }

            return true;

        }

    }


}
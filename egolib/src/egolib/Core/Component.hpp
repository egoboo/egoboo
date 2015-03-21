#pragma once

#include "egolib/platform.h"
#include "egolib/log.h"

namespace Component
{
    /**
     * @brief
     *  A factory for a component.
     * @author
     *  Michael Heilmann
     */
    template <typename _Type>
    class Factory
    {
        /**
         * @brief
         *  Get a human-readable name of the component.
         * @return
         *  a human-readable name of the component
         */
        std::string getName() const;
        /**
         * @brief
         *  Create an instance of the component.
         * @return
         *  an instance of the component
         */
        _Type *create();
    };

    /**
     * @brief
     *  Singleton pattern for components.
     * @author
     *  Michael Heilmann
     */
    template <typename _Type, typename _Factory = Factory<_Type>>
    class Singleton
    {

    private:

        /**
        * @brief
        *  The component's singleton instance.
        */
        static _Type *_singleton;

    protected:

        /**
        * @brief
        *  Construct this singleton.
        * @remark
        *  Intentionally protected.
        */
        Singleton()
        {}

        /**
        * @brief
        *  Destruct this singleton.
        * @remark
        *  Intentionally protected.
        */
        virtual ~Singleton()
        {}

    public:

        /**
        * @brief
        *  Get this singleton.
        * @return
        *  the singleton
        * @throw std::logic_error
        *  if the singleton is not initialized
        * @pre
        *	The singleton must be initialized.
        * @warning
        *	Uninitializing the singleton will invalidate any references returned by calls to this method prior to uninitialization.
        */
        static _Type& get()
        {
            if (!_singleton)
            {
                std::ostringstream msg;
                msg << __FILE__ << ":" << __LINE__ << ": " << _Factory().getName() << " not initialized - ignoring";
                throw std::logic_error(msg.str());
            }
            return *_singleton;
        }

        /**
        * @brief
        *  Initialize this component.
        * @return
        *  @a true on success, @a false on failure
        * @remark
        *  A call to this method results in a warning and succeeds if the component is already initialized.
        * @post
        *  The component is initialized.
        */
        static bool initialize()
        {
            if (_singleton)
            {
                std::ostringstream msg;
                msg << __FILE__ << ":" << __LINE__ << ": " << _Factory().getName() << " already initialized - ignoring";
                log_warning("%s\n", msg.str().c_str());
                return true;
            }
            try
            {
                _singleton = _Factory().create();
            }
            catch (std::exception&)
            {
                std::ostringstream msg;
                msg << __FILE__ << ":" << __LINE__ << ": " << " unable to initialize " << _Factory().getName();
                log_warning("%s\n", msg.str().c_str());
                return false;
            }
            return true;
        }

        /**
        * @brief
        *  Uninitialize this singleton.
        * @remark
        *  A call to this method results in a warning if the singleton is not initialized.
        * @post
        *  The singleton is not initialized.
        */
        static void uninitialize()
        {
            if (!_singleton)
            {
                std::ostringstream msg;
                msg << __FILE__ << ":" << __LINE__ << ": " << _Factory().getName() << " not initialized - ignoring";
                log_warning("%s\n", msg.str().c_str());
                return;
            }
            delete _singleton;
            _singleton = nullptr;
        }
    };

    template <typename _Type, typename _Factory>
    _Type *Singleton<_Type, _Factory>::_singleton = nullptr;

};
